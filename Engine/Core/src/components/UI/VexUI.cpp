#define STB_TRUETYPE_IMPLEMENTATION

#include "glm/ext/matrix_clip_space.hpp"
#include <climits>
#include <components/UI/VexUI.hpp>
#include <components/UI/UIVertex.hpp>
#include <glm/glm.hpp>

#include <components/errorUtils.hpp>
#include <components/pathUtils.hpp>

namespace vex {

Widget::~Widget() {
    if (yoga) YGNodeFree(yoga);
}

void Widget::applyJson(const nlohmann::json& j) {
    if (j.contains("id"))      id = j["id"].get<std::string>();
    if (j.contains("text"))    text = j["text"].get<std::string>();
    if (j.contains("image"))   image = j["image"].get<std::string>();

    if (j.contains("size") && j["size"].is_array() && j["size"].size() == 2) {
        size.x = j["size"][0].get<float>();
        size.y = j["size"][1].get<float>();
    }

    if (j.contains("style")) {
        const auto& s = j["style"];
        if (s.contains("color") && s["color"].is_array() && s["color"].size() == 4) {
            style.color = glm::vec4(s["color"][0].get<float>(), s["color"][1].get<float>(), s["color"][2].get<float>(), s["color"][3].get<float>());
        }
        if (s.contains("bgColor") && s["bgColor"].is_array() && s["bgColor"].size() == 4) {
            style.bgColor = glm::vec4(s["bgColor"][0].get<float>(), s["bgColor"][1].get<float>(), s["bgColor"][2].get<float>(), s["bgColor"][3].get<float>());
        }
        if (s.contains("font")) style.font = s["font"].get<std::string>();
        if (s.contains("size")) style.fontSize = s["size"].get<float>();
    }
}

VexUI::VexUI(VulkanContext& ctx, VirtualFileSystem* vfs, VulkanResources* res)
    : m_ctx(ctx), m_vfs(vfs), m_res(res) {}

VexUI::~VexUI() {
    freeTree(m_root);
    for (auto& [k, a] : m_fontAtlases) {
        if (a.view) vkDestroyImageView(m_ctx.device, a.view, nullptr);
        if (a.image) vmaDestroyImage(m_ctx.allocator, a.image, a.alloc);
    }
    if (m_uiSampler) vkDestroySampler(m_ctx.device, m_uiSampler, nullptr);
    if (m_vb) vmaDestroyBuffer(m_ctx.allocator, m_vb, m_vbAlloc);
}

bool VexUI::init() {
    const size_t VB_BYTES = 2 * 1024 * 1024;
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bi.size = VB_BYTES;
    bi.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VmaAllocationCreateInfo ai{};
    ai.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    vmaCreateBuffer(m_ctx.allocator, &bi, &ai, &m_vb, &m_vbAlloc, nullptr);
    m_vbSize = VB_BYTES;

    VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    si.magFilter = si.minFilter = VK_FILTER_NEAREST;
    si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(m_ctx.device, &si, nullptr, &m_uiSampler);
    initialized = true;
    return true;
}

void VexUI::loadFonts(Widget* w) {
    if (!w) return;

    if (!w->style.font.empty() && w->style.fontSize > 0.f) {
        std::string key = w->style.font + "_" + std::to_string(static_cast<int>(w->style.fontSize));
        if (m_fontAtlases.count(key) > 0) goto recurse;

        auto data = m_vfs->load_file(GetAssetPath(w->style.font));
        if (!data) {
            log("UI: cannot load font %s", w->style.font.c_str());
            goto recurse;
        }

        FontAtlas atlas;
        if (!stbtt_InitFont(&atlas.info, (unsigned char*)data->data.data(), 0)) {
            log("UI: stbtt_InitFont failed for %s", w->style.font.c_str());
            goto recurse;
        }

        int ascent, descent, lineGap;
        stbtt_GetFontVMetrics(&atlas.info, &ascent, &descent, &lineGap); // Get metrics
        log("UI: font %s metrics: ascent=%d, descent=%d, lineGap=%d", w->style.font.c_str(), ascent, descent, lineGap);

        float scale = stbtt_ScaleForPixelHeight(&atlas.info, w->style.fontSize); // Get scale
        log("UI: font %s scale=%f", w->style.font.c_str(), scale);

        atlas.ascent = static_cast<float>(ascent) * scale;
        atlas.descent = static_cast<float>(descent) * scale;
        atlas.scale = scale;

        const int W = 512, H = 512;
        std::vector<unsigned char> bitmap(W * H, 0);
        atlas.cdata.resize(96);
        stbtt_BakeFontBitmap((unsigned char*)data->data.data(), 0, w->style.fontSize, bitmap.data(), W, H, 32, 96, atlas.cdata.data());

        std::vector<unsigned char> rgba(W * H * 4);
        for (int i = 0; i < W * H; ++i) {
            unsigned char v = bitmap[i];
            rgba[i*4 + 0] = 255;
            rgba[i*4 + 1] = 255;
            rgba[i*4 + 2] = 255;
            rgba[i*4 + 3] = v;
        }

        std::string texName = "ui_font_" + key;
        m_res->createTextureFromRaw(rgba, W, H, texName);

        atlas.texIdx = m_res->getTextureIndex(texName);
        atlas.width = W;
        atlas.height = H;
        atlas.bakedSize = w->style.fontSize;
        m_fontAtlases[key] = atlas;
    }

recurse:
    for (auto* c : w->children) loadFonts(c);
}

void VexUI::loadImages(Widget* w) {
    if (!w) return;
    if (!w->image.empty()) {
        m_res->loadTexture(GetAssetPath(w->image), GetAssetPath(w->image));
    }
    for (auto* c : w->children) loadImages(c);
}

// @todo Implement more of yogas features like margin, flexes?, aligments.
Widget* VexUI::parseNode(const nlohmann::json& j) {
    Widget* w = new Widget();
    w->type = WidgetType::Container;

    if (j.contains("type")) {
        std::string t = j["type"].get<std::string>();
        if (t == "label") w->type = WidgetType::Label;
        else if (t == "image") w->type = WidgetType::Image;
        else if (t == "button") w->type = WidgetType::Button;
    }

    w->applyJson(j);

    w->ui = this;
    w->yoga = YGNodeNew();
    YGNodeSetContext(w->yoga, w);

    if (j.contains("layout")) {
        std::string l = j["layout"].get<std::string>();
        if (l == "row") YGNodeStyleSetFlexDirection(w->yoga, YGFlexDirectionRow);
        else if (l == "column") YGNodeStyleSetFlexDirection(w->yoga, YGFlexDirectionColumn);
    }
    if (j.contains("justify")) {
        std::string jst = j["justify"].get<std::string>();
        if (jst == "space-between") YGNodeStyleSetJustifyContent(w->yoga, YGJustifySpaceBetween);
        else if (jst == "center") YGNodeStyleSetJustifyContent(w->yoga, YGJustifyCenter);
        else if (jst == "space-evenly") YGNodeStyleSetJustifyContent(w->yoga, YGJustifySpaceEvenly);
        else if (jst == "space-around") YGNodeStyleSetJustifyContent(w->yoga, YGJustifySpaceAround);
        else if (jst == "flex-end") YGNodeStyleSetJustifyContent(w->yoga, YGJustifyFlexEnd);
        else if (jst == "flex-start") YGNodeStyleSetJustifyContent(w->yoga, YGJustifyFlexStart);
    }
    if (j.contains("align")) {
        std::string al = j["align"].get<std::string>();
        if (al == "center") YGNodeStyleSetAlignItems(w->yoga, YGAlignCenter);
        else if (al == "auto") YGNodeStyleSetAlignItems(w->yoga, YGAlignAuto);
        else if (al == "flex-start") YGNodeStyleSetAlignItems(w->yoga, YGAlignFlexStart);
        else if (al == "flex-end") YGNodeStyleSetAlignItems(w->yoga, YGAlignFlexEnd);
        else if (al == "stretch") YGNodeStyleSetAlignItems(w->yoga, YGAlignStretch);
        else if (al == "baseline") YGNodeStyleSetAlignItems(w->yoga, YGAlignBaseline);
        else if (al == "space-around") YGNodeStyleSetAlignItems(w->yoga, YGAlignSpaceAround);
        else if (al == "space-between") YGNodeStyleSetAlignItems(w->yoga, YGAlignSpaceBetween);
        else if (al == "space-evenly") YGNodeStyleSetAlignItems(w->yoga, YGAlignSpaceEvenly);
    }

    if (j.contains("padding")) YGNodeStyleSetPadding(w->yoga, YGEdgeAll, j["padding"].get<float>());

    if (j.contains("size")) {
            YGNodeStyleSetWidth(w->yoga, w->size.x);
            YGNodeStyleSetHeight(w->yoga, w->size.y);
        }

    if (j.contains("margin")) {
            YGNodeStyleSetMargin(w->yoga, YGEdgeAll, j["margin"].get<float>());
        }

    if (j.contains("flexGrow")) YGNodeStyleSetFlexGrow(w->yoga, j["flexGrow"].get<float>());
    if (j.contains("flexShrink")) YGNodeStyleSetFlexShrink(w->yoga, j["flexShrink"].get<float>());
    if (j.contains("flexBasis")) YGNodeStyleSetFlexBasis(w->yoga, j["flexBasis"].get<float>());

    if (j.contains("position")) {
        std::string pos = j["position"].get<std::string>();
        if (pos == "absolute"){
            YGNodeStyleSetPositionType(w->yoga, YGPositionTypeAbsolute);

            if (j.contains("left")) YGNodeStyleSetPosition(w->yoga, YGEdgeLeft, j["left"].get<float>());
            if (j.contains("right")) YGNodeStyleSetPosition(w->yoga, YGEdgeRight, j["right"].get<float>());
            if (j.contains("top")) YGNodeStyleSetPosition(w->yoga, YGEdgeTop, j["top"].get<float>());
            if (j.contains("bottom")) YGNodeStyleSetPosition(w->yoga, YGEdgeBottom, j["bottom"].get<float>());
        }
        else if (pos == "relative") YGNodeStyleSetPositionType(w->yoga, YGPositionTypeRelative);
    }

    if (j.contains("wrap")) {
        std::string wr = j["wrap"].get<std::string>();
        if (wr == "wrap") YGNodeStyleSetFlexWrap(w->yoga, YGWrapWrap);
        else if (wr == "no-wrap") YGNodeStyleSetFlexWrap(w->yoga, YGWrapNoWrap);
        else if (wr == "wrap-reverse") YGNodeStyleSetFlexWrap(w->yoga, YGWrapWrapReverse);
    }

    if (j.contains("alignSelf")) {
        std::string as = j["alignSelf"].get<std::string>();
        if (as == "center") YGNodeStyleSetAlignSelf(w->yoga, YGAlignCenter);
        else if (as == "auto") YGNodeStyleSetAlignSelf(w->yoga, YGAlignAuto);
        else if (as == "flex-start") YGNodeStyleSetAlignSelf(w->yoga, YGAlignFlexStart);
        else if (as == "flex-end") YGNodeStyleSetAlignSelf(w->yoga, YGAlignFlexEnd);
        else if (as == "stretch") YGNodeStyleSetAlignSelf(w->yoga, YGAlignStretch);
        else if (as == "baseline") YGNodeStyleSetAlignSelf(w->yoga, YGAlignBaseline);
        else if (as == "space-around") YGNodeStyleSetAlignSelf(w->yoga, YGAlignSpaceAround);
        else if (as == "space-between") YGNodeStyleSetAlignSelf(w->yoga, YGAlignSpaceBetween);
        else if (as == "space-evenly") YGNodeStyleSetAlignSelf(w->yoga, YGAlignSpaceEvenly);
    }

    if (j.contains("paddingLeft")) YGNodeStyleSetPadding(w->yoga, YGEdgeLeft, j["paddingLeft"].get<float>());
    if (j.contains("paddingRight")) YGNodeStyleSetPadding(w->yoga, YGEdgeRight, j["paddingRight"].get<float>());
    if (j.contains("paddingTop")) YGNodeStyleSetPadding(w->yoga, YGEdgeTop, j["paddingTop"].get<float>());
    if (j.contains("paddingBottom")) YGNodeStyleSetPadding(w->yoga, YGEdgeBottom, j["paddingBottom"].get<float>());

    if (j.contains("marginLeft")) YGNodeStyleSetMargin(w->yoga, YGEdgeLeft, j["marginLeft"].get<float>());
    if (j.contains("marginRight")) YGNodeStyleSetMargin(w->yoga, YGEdgeRight, j["marginRight"].get<float>());
    if (j.contains("marginTop")) YGNodeStyleSetMargin(w->yoga, YGEdgeTop, j["marginTop"].get<float>());
    if (j.contains("marginBottom")) YGNodeStyleSetMargin(w->yoga, YGEdgeBottom, j["marginBottom"].get<float>());

    if (j.contains("borderWidth")) {
        w->style.borderWidth = j["borderWidth"].get<float>();
        YGNodeStyleSetBorder(w->yoga, YGEdgeAll, w->style.borderWidth);  // Affects layout space
    }
    if (j.contains("borderColor") && j["borderColor"].is_array() && j["borderColor"].size() == 4) {
        w->style.borderColor = glm::vec4(j["borderColor"][0].get<float>(), j["borderColor"][1].get<float>(),
                                         j["borderColor"][2].get<float>(), j["borderColor"][3].get<float>());
    }

    if (j.contains("textAlign")) {
        std::string ta = j["textAlign"].get<std::string>();
        if (ta == "center") w->textAlign = TextAlign::Center;
        else if (ta == "right") w->textAlign = TextAlign::Right;
    }

    if (j.contains("children") && j["children"].is_array()) {
        for (const auto& c : j["children"]) {
            Widget* child = parseNode(c);
            auto childYoga = child->yoga;
            if (c.contains("size") && c["size"].is_array() && c["size"].size() == 2) {
                YGNodeStyleSetWidth(childYoga, child->size.x);
                YGNodeStyleSetHeight(childYoga, child->size.y);
            }
            else if ((child->type == WidgetType::Label || child->type == WidgetType::Button) && !child->text.empty()) {
                YGNodeSetMeasureFunc(childYoga, VexUI::measureTextNode);
            }

            if (!child->children.empty()) {
                YGAlign alignSelf = YGNodeStyleGetAlignSelf(childYoga);
                if (alignSelf == YGAlignAuto) {
                    YGNodeStyleSetAlignSelf(childYoga, YGAlignStretch);
                }
            }

            w->children.push_back(child);
            YGNodeInsertChild(w->yoga, child->yoga, static_cast<uint32_t>(w->children.size() - 1));
        }
    }
    return w;
}

void VexUI::load(const std::string& path) {
    if(initialized){
        std::string realPath = GetAssetPath(path);
        auto data = m_vfs->load_file(realPath);
        if (!data) { log("UI: cannot open %s", realPath.c_str()); return; }

        nlohmann::json json;
        try { json = nlohmann::json::parse(data->data.begin(), data->data.end()); }
        catch (const nlohmann::json::parse_error& e) { log("UI JSON error: %s", e.what()); return; }

        freeTree(m_root);
        m_root = nullptr;
        if (json.contains("root")) {
            m_root = parseNode(json["root"]);
            if (json.contains("zindex")){
                zIndex = json["zindex"].get<int>();
            }
        }
        if (m_root && json.contains("overlays") && json["overlays"].is_array()) {
            for (const auto& ov : json["overlays"]) {
                Widget* overlay = parseNode(ov);
                YGNodeStyleSetPositionType(overlay->yoga, YGPositionTypeAbsolute);
                m_root->children.push_back(overlay);
                YGNodeInsertChild(m_root->yoga, overlay->yoga, static_cast<uint32_t>(m_root->children.size() - 1));
            }
        }
        if (m_root) {
            loadFonts(m_root);
            loadImages(m_root);
        }
    }else{
        loadPending = true;
        loadPath = path;
    }
}

void VexUI::layout(glm::uvec2 res) {
    if (m_root) YGNodeCalculateLayout(m_root->yoga, res.x, res.y, YGDirectionLTR);
}

Widget* VexUI::findWidgetAt(Widget* w, glm::vec2 pos) {
    if (!w) return nullptr;
    float x = YGNodeLayoutGetLeft(w->yoga);
    float y = YGNodeLayoutGetTop(w->yoga);
    float width  = YGNodeLayoutGetWidth(w->yoga);
    float height = YGNodeLayoutGetHeight(w->yoga);
    if (pos.x >= x && pos.x <= x+width && pos.y >= y && pos.y <= y+height) {
        for (auto it = w->children.rbegin(); it != w->children.rend(); ++it) {
            if (auto* hit = findWidgetAt(*it, pos)) return hit;
        }
        return w;
    }
    return nullptr;
}

Widget* VexUI::findById(Widget* w, const std::string& id) {
    if (!w) return nullptr;
    if (w->id == id) return w;
    for (auto* c : w->children) if (auto* f = findById(c, id)) return f;
    return nullptr;
}

void VexUI::setText(const std::string& id, const std::string& txt) {
    if (initialized){
        if (Widget* w = findById(m_root, id)) w->text = txt;
    }else{
        pendingSetters.push_back([this, id, txt]() {
            if (Widget* w = findById(m_root, id)) w->text = txt;
        });
    }
}

void VexUI::setOnClick(const std::string& id, std::function<void()> cb) {
    if (initialized){
        if (Widget* w = findById(m_root, id); w && w->type == WidgetType::Button)
            w->onClick = std::move(cb);
    }else{
        pendingSetters.push_back([this, id, cb]() {
            if (Widget* w = findById(m_root, id); w && w->type == WidgetType::Button)
                w->onClick = std::move(cb);
        });
    }
}

void VexUI::processEvent(const SDL_Event& ev) {
    if (!m_root) return;
    float sx = static_cast<float>(m_ctx.swapchainExtent.width)  / m_ctx.currentRenderResolution.x;
    float sy = static_cast<float>(m_ctx.swapchainExtent.height) / m_ctx.currentRenderResolution.y;

    if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        log("Mouse button down. pos: (%f, %f)", ev.button.x / sx, ev.button.y / sy);
        glm::vec2 mouse(ev.button.x / sx, ev.button.y / sy);
        if (Widget* w = findWidgetAt(m_root, mouse); w && w->type == WidgetType::Button && w->onClick)
            w->onClick();
    }
}

YGSize VexUI::calculateTextSize(Widget* w, float maxWidth) {
    std::string key = w->style.font + "_" + std::to_string(static_cast<int>(w->style.fontSize));
    auto it = m_fontAtlases.find(key);
    if (it == m_fontAtlases.end() || w->text.empty()) {
        return {0, 0};
    }
    const FontAtlas& a = it->second;

    std::vector<std::string> lines;
    std::string currentLine;
    float currentWidth = 0.f;
    for (char ch : w->text) {
        if (ch == '\n') {
            lines.push_back(currentLine);
            currentLine.clear();
            currentWidth = 0.f;
            continue;
        }
        if (ch < 32 || ch > 127) continue;
        const stbtt_bakedchar& cd = a.cdata[ch - 32];
        if (currentWidth + cd.xadvance > maxWidth && !currentLine.empty()) {
            lines.push_back(currentLine);
            currentLine.clear();
            currentWidth = 0.f;
        }
        currentLine += ch;
        currentWidth += cd.xadvance;
    }
    if (!currentLine.empty()) lines.push_back(currentLine);float lineHeight = a.ascent - a.descent;
        float totalHeight = lines.size() * lineHeight;
        float totalWidth = 0.f;
        for (const auto& line : lines) {
            float lineWidth = 0.f;
            for (char ch : line) {
                if (ch < 32 || ch > 127) continue;
                const stbtt_bakedchar& cd = a.cdata[ch - 32];
                lineWidth += cd.xadvance;
            }
            totalWidth = std::max(totalWidth, lineWidth);
        }
return {totalWidth, totalHeight};
}

YGSize VexUI::measureTextNode(const YGNode* node, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
    Widget* w = static_cast<Widget*>(YGNodeGetContext(node));
    if (!w || !w->ui) return {0, 0};

    float maxW = FLT_MAX;
    if (widthMode == YGMeasureModeExactly || widthMode == YGMeasureModeAtMost) {
        maxW = width;
    }

    YGSize naturalSize = w->ui->calculateTextSize(w, maxW);
    YGSize measuredSize;

    if (widthMode == YGMeasureModeExactly) {
        measuredSize.width = width;
    } else if (widthMode == YGMeasureModeAtMost) {
        measuredSize.width = std::min(width, naturalSize.width);
    } else {
        measuredSize.width = naturalSize.width;
    }

    if (heightMode == YGMeasureModeExactly) {
        measuredSize.height = height;
    } else if (heightMode == YGMeasureModeAtMost) {
        measuredSize.height = std::min(height, naturalSize.height);
    } else {
        measuredSize.height = naturalSize.height;
    }

    return measuredSize;
}

void VexUI::batch(Widget* w, std::vector<float>& verts, Widget* parent) {
    if (!w) return;
    float x = YGNodeLayoutGetLeft(w->yoga);
    float y = YGNodeLayoutGetTop(w->yoga);
    if (parent && YGNodeStyleGetPositionType(w->yoga) != YGPositionTypeAbsolute) {
        x += YGNodeLayoutGetLeft(parent->yoga);// - YGNodeLayoutGetPadding(parent->yoga, YGEdgeLeft);
        y += YGNodeLayoutGetTop(parent->yoga);// - YGNodeLayoutGetPadding(parent->yoga, YGEdgeTop);
    }
    float width  = YGNodeLayoutGetWidth(w->yoga);
    float height = YGNodeLayoutGetHeight(w->yoga);

    auto pushQuad = [&](float x0, float y0, float u0, float v0,
                        float x1, float y1, float u1, float v1,
                        const glm::vec4& col, float texIdx) {
        // Two triangles to make a quad (6 vertices)
        // Triangle 1
        verts.insert(verts.end(), {x0, y0, u0, v0, col.r, col.g, col.b, col.a, texIdx});
        verts.insert(verts.end(), {x1, y0, u1, v0, col.r, col.g, col.b, col.a, texIdx});
        verts.insert(verts.end(), {x0, y1, u0, v1, col.r, col.g, col.b, col.a, texIdx});

        // Triangle 2
        verts.insert(verts.end(), {x1, y0, u1, v0, col.r, col.g, col.b, col.a, texIdx});
        verts.insert(verts.end(), {x1, y1, u1, v1, col.r, col.g, col.b, col.a, texIdx});
        verts.insert(verts.end(), {x0, y1, u0, v1, col.r, col.g, col.b, col.a, texIdx});
    };

    if (w->style.bgColor.a > 0.f) {
        pushQuad(x, y, 0, 0, x + width, y + height, 1, 1, w->style.bgColor, -1.f);
    }

    if (w->style.borderWidth > 0.f && w->style.borderColor.a > 0.f) {
        // Top
        pushQuad(x, y, 0, 0, x + width, y + w->style.borderWidth, 1, 1, w->style.borderColor, -1.f);
        // Bottom
        pushQuad(x, y + height - w->style.borderWidth, 0, 0, x + width, y + height, 1, 1, w->style.borderColor, -1.f);
        // Left
        pushQuad(x, y, 0, 0, x + w->style.borderWidth, y + height, 1, 1, w->style.borderColor, -1.f);
        // Right
        pushQuad(x + width - w->style.borderWidth, y, 0, 0, x + width, y + height, 1, 1, w->style.borderColor, -1.f);
    }

    if ((w->type == WidgetType::Label || w->type == WidgetType::Button) && !w->text.empty() && !w->style.font.empty()) {
        std::string key = w->style.font + "_" + std::to_string(static_cast<int>(w->style.fontSize));
        auto it = m_fontAtlases.find(key);
        if (it != m_fontAtlases.end()) {
            const FontAtlas& a = it->second;

            std::vector<std::string> lines;
                    std::string currentLine;
                    float currentWidth = 0.f;
                    for (char ch : w->text) {
                        if (ch == '\n') {
                            lines.push_back(currentLine);
                            currentLine.clear();
                            currentWidth = 0.f;
                            continue;
                        }
                        if (ch < 32 || ch > 127) continue;
                        const stbtt_bakedchar& cd = a.cdata[ch - 32];
                        if (currentWidth + cd.xadvance > width && !currentLine.empty()) {
                            lines.push_back(currentLine);
                            currentLine.clear();
                            currentWidth = 0.f;
                        }
                        currentLine += ch;
                        currentWidth += cd.xadvance;
                    }
                    if (!currentLine.empty()) lines.push_back(currentLine);

                    float lineHeight = a.ascent - a.descent;
                    float cy = y + a.ascent;

                    for (const auto& line : lines) {
                        float lineWidth = 0.f;
                        for (char ch : line) {
                            if (ch < 32 || ch > 127) continue;
                            const stbtt_bakedchar& cd = a.cdata[ch - 32];
                            lineWidth += cd.xadvance;
                        }

                        float startX = x;
                        if (w->textAlign == TextAlign::Center) {
                            startX = x + (width - lineWidth) / 2.f;
                        } else if (w->textAlign == TextAlign::Right) {
                            startX = x + (width - lineWidth);
                        }

                        float cx = startX;
                        for (char ch : line) {
                            if (ch < 32 || ch > 127) continue;
                            const stbtt_bakedchar& cd = a.cdata[ch - 32];

                            float px = cx + cd.xoff;
                            float py = cy + cd.yoff;

                            float u0 = cd.x0 / float(a.width);
                            float v0 = cd.y0 / float(a.height);
                            float u1 = cd.x1 / float(a.width);
                            float v1 = cd.y1 / float(a.height);

                            pushQuad(px, py, u0, v0,
                                     px + (cd.x1 - cd.x0), py + (cd.y1 - cd.y0), u1, v1,
                                     w->style.color, static_cast<float>(a.texIdx));
                            cx += cd.xadvance;
                        }
                        cy += lineHeight;
            }
        }
    }

    if (w->type == WidgetType::Image && !w->image.empty()) {
        uint32_t idx = m_res->getTextureIndex(GetAssetPath(w->image));
        if (idx != UINT32_MAX) {
            pushQuad(x, y, 0, 0, x + width, y + height, 1, 1, {1,1,1,1}, static_cast<float>(idx));
        }else{
            log("Failed to find image: %s", w->image.c_str());
        }
    }

    for (auto* c : w->children) batch(c, verts, w);
}

void VexUI::render(VkCommandBuffer cmd, VkPipeline pipeline, VkPipelineLayout pipelineLayout, int currentFrame) {
    if (!m_root) return;
    layout(m_ctx.currentRenderResolution);

    vkDeviceWaitIdle(m_ctx.device);

    std::vector<float> verts;
    verts.reserve(1024 * 9);
    batch(m_root, verts);
    uploadVerts(verts);

    if (verts.empty()) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkDescriptorSet globalUBO = m_res->getUBODescriptorSet(currentFrame);
        if (globalUBO != VK_NULL_HANDLE) {
            // We must provide an offset because Binding 1 is DYNAMIC.
            // We use 0 since the UI doesn't actually read lights, but the API demands a valid offset.
            uint32_t dynamicOffset = 0;

            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout,
                0,
                1, &globalUBO,
                1, &dynamicOffset // <--- Changed from 0, nullptr
            );
        }

    glm::mat4 ortho = glm::ortho(
        0.0f, static_cast<float>(m_ctx.currentRenderResolution.x),
        0.0f, static_cast<float>(m_ctx.currentRenderResolution.y),
        -1.0f, 1.0f
    );

    UIPushConstants uiPC{ ortho };

    vkCmdPushConstants(
        cmd,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(UIPushConstants),
        &uiPC);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_vb, &offset);

    const uint32_t verticesPerQuad = 6;
    const uint32_t floatsPerVertex = 9;
    const uint32_t floatsPerQuad = verticesPerQuad * floatsPerVertex;

    uint32_t totalVertices = verts.size() / floatsPerVertex;
    uint32_t currentVertex = 0;

    int currentTexIndex = INT_MIN;
    VkDescriptorSet currentTexSet = VK_NULL_HANDLE;

    while (currentVertex < totalVertices) {
        uint32_t quadStart = currentVertex * floatsPerVertex;
        float texIndexFloat = verts[quadStart + 8];

        int texIndex = static_cast<int>(texIndexFloat);

        if (texIndex != currentTexIndex) {
            currentTexIndex = texIndex;

            if (texIndex >= 0) {
                currentTexSet = m_res->getTextureDescriptorSet(currentFrame, texIndex);
            } else {
                currentTexSet = m_res->getTextureDescriptorSet(currentFrame, 0);
            }

            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout,
                1,
                1, &currentTexSet,
                0, nullptr
            );
        }

        vkCmdDraw(cmd, 6, 1, currentVertex, 0);
        currentVertex += 6;
    }
}

void VexUI::uploadVerts(const std::vector<float>& verts) {
    if (verts.empty()) return;
    size_t bytes = verts.size() * sizeof(float);
    if (bytes > m_vbSize) throw_error("UI VB overflow");

    void* dst; vmaMapMemory(m_ctx.allocator, m_vbAlloc, &dst);
    memcpy(dst, verts.data(), bytes);
    vmaUnmapMemory(m_ctx.allocator, m_vbAlloc);
}

void VexUI::freeTree(Widget* w) {
    if (!w) return;
    for (auto* c : w->children) freeTree(c);
    delete w;
}
}
