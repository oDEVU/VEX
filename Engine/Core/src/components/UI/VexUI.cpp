#define STB_TRUETYPE_IMPLEMENTATION

#include "glm/ext/matrix_clip_space.hpp"
#include <climits>
#include <components/UI/VexUI.hpp>
#include <components/UI/UIVertex.hpp>
#include <glm/glm.hpp>
#include <immintrin.h>

#include <components/ErrorUtils.hpp>
#include <components/PathUtils.hpp>
#include "components/HardwareInfo.hpp"

namespace vex {

std::vector<std::string> VexUI::wrapText(const std::string& text, const FontAtlas& a, float maxWidth) {
    std::vector<std::string> lines;
    std::string currentLine;
    float currentWidth = 0.f;
    size_t lastSpacePos = std::string::npos;

    for (char ch : text) {
        if (ch == '\n') {
            lines.push_back(currentLine);
            currentLine.clear();
            currentWidth = 0.f;
            lastSpacePos = std::string::npos;
            continue;
        }

        if (ch < 32 || ch > 127) continue;

        const stbtt_bakedchar& cd = a.cdata[ch - 32];
        float advance = cd.xadvance;

        if (currentWidth + advance > maxWidth && !currentLine.empty()) {
            if (ch == ' ') {
                lines.push_back(currentLine);
                currentLine.clear();
                currentWidth = 0.f;
                lastSpacePos = std::string::npos;
            } else if (lastSpacePos != std::string::npos) {
                lines.push_back(currentLine.substr(0, lastSpacePos));

                std::string remainder = currentLine.substr(lastSpacePos + 1);
                currentLine = remainder + ch;

                currentWidth = 0.f;
                for (char r : currentLine) {
                    currentWidth += a.cdata[r - 32].xadvance;
                }
                lastSpacePos = std::string::npos;
            } else {
                lines.push_back(currentLine);
                currentLine = std::string(1, ch);
                currentWidth = advance;
                lastSpacePos = std::string::npos;
            }
        } else {
            if (ch == ' ') {
                lastSpacePos = currentLine.length();
            }
            currentLine += ch;
            currentWidth += advance;
        }
    }
    if (!currentLine.empty()) lines.push_back(currentLine);

    return lines;
}

    static void rotateQuadScalar(float* outVerts, float pivotX, float pivotY,
                           float sinA, float cosA, float x0, float y0, float x1, float y1) {
        float px[] = {x0, x1, x0, x1};
        float py[] = {y0, y0, y1, y1};

        for(int i=0; i<4; ++i) {
            float dx = px[i] - pivotX;
            float dy = py[i] - pivotY;
            outVerts[i*2+0] = pivotX + (dx * cosA - dy * sinA);
            outVerts[i*2+1] = pivotY + (dx * sinA + dy * cosA);
        }
    }

    __attribute__((target("avx2")))
    static void rotateQuadAvX2(float* outVerts, float pivotX, float pivotY,
                         float sinA, float cosA, float x0, float y0, float x1, float y1) {

        __m256 vPos = _mm256_setr_ps(x0, y0, x1, y0, x0, y1, x1, y1);
        __m256 vPivot = _mm256_setr_ps(pivotX, pivotY, pivotX, pivotY, pivotX, pivotY, pivotX, pivotY);
        __m256 vDelta = _mm256_sub_ps(vPos, vPivot);
        __m256 vCos = _mm256_set1_ps(cosA);
        __m256 vSin = _mm256_set1_ps(sinA);
        __m256 vDeltaSwapped = _mm256_permute_ps(vDelta, 0xB1);
        __m256 t1 = _mm256_mul_ps(vDelta, vCos);
        __m256 t2 = _mm256_mul_ps(vDeltaSwapped, vSin);
        __m256 vResult = _mm256_addsub_ps(t1, t2);
        vResult = _mm256_add_ps(vResult, vPivot);

        _mm256_storeu_ps(outVerts, vResult);
    }

void UIUnitValue::parse(const nlohmann::json& jval) {
    if (jval.is_number()) {
        value = jval.get<float>();
        type = UIUnitType::Psp;
    } else if (jval.is_string()) {
        std::string s = jval.get<std::string>();
        if (s == "auto") { value = 0.f; type = UIUnitType::Auto; return; }

        size_t pos = 0;
        try { value = std::stof(s, &pos); } catch(...) { value = 0.f; type = UIUnitType::Auto; return; }

        if (pos >= s.length()) { type = UIUnitType::Psp; return; }

        std::string unit = s.substr(pos);
        unit.erase(0, unit.find_first_not_of(" \t"));
        unit.erase(unit.find_last_not_of(" \t") + 1);

        if (unit == "px") type = UIUnitType::Px;
        else if (unit == "%") type = UIUnitType::Percent;
        else if (unit == "vw") type = UIUnitType::Vw;
        else if (unit == "vh") type = UIUnitType::Vh;
        else type = UIUnitType::Psp;
    }
}

nlohmann::json UIUnitValue::toJson() const {
    if (type == UIUnitType::Auto) return "auto";
    if (type == UIUnitType::Psp) return value;

    std::string s = std::to_string(value);
    s.erase(s.find_last_not_of('0') + 1, std::string::npos);
    if (s.back() == '.') s.pop_back();

    if (type == UIUnitType::Px) return s + "px";
    if (type == UIUnitType::Percent) return s + "%";
    if (type == UIUnitType::Vw) return s + "vw";
    if (type == UIUnitType::Vh) return s + "vh";
    return value;
}

float UIUnitValue::getPixels(const VexUI* ui) const {
    if (!ui) return value;
    switch (type) {
        case UIUnitType::Px: return value;
        case UIUnitType::Psp: return value * ui->getPspMultiplier();
        case UIUnitType::Vw: return value * (ui->getRenderResolution().x / 100.0f);
        case UIUnitType::Vh: return value * (ui->getRenderResolution().y / 100.0f);
        case UIUnitType::Percent: return value;
        case UIUnitType::Auto: return 0.f;
    }
    return 0.f;
}

Widget::~Widget() {
    if (yoga) YGNodeFree(yoga);
}

void Widget::applyLayout(VexUI* uiManager) {
    if (!uiManager || !yoga) return;

    if (nodeJson.contains("type")) {
        std::string t = nodeJson["type"].get<std::string>();
        if (t == "label") type = WidgetType::Label;
        else if (t == "image") type = WidgetType::Image;
        else if (t == "button") type = WidgetType::Button;
    }

    if (nodeJson.contains("id"))      id = nodeJson["id"].get<std::string>();
    if (nodeJson.contains("text"))    text = nodeJson["text"].get<std::string>();
    if (nodeJson.contains("image"))   image = nodeJson["image"].get<std::string>();

    if (nodeJson.contains("size") && nodeJson["size"].is_array() && nodeJson["size"].size() == 2) {
        size.x = UIUnitValue::parseJson(nodeJson["size"][0]);
        size.y = UIUnitValue::parseJson(nodeJson["size"][1]);
        uiManager->applyYogaDimension(yoga, size.x, YGNodeStyleSetWidth, YGNodeStyleSetWidthPercent, YGNodeStyleSetWidthAuto);
        uiManager->applyYogaDimension(yoga, size.y, YGNodeStyleSetHeight, YGNodeStyleSetHeightPercent, YGNodeStyleSetHeightAuto);
    }

    if (nodeJson.contains("style")) {
        const auto& s = nodeJson["style"];
        if (s.contains("color") && s["color"].is_array() && s["color"].size() == 4) {
            style.color = glm::vec4(s["color"][0].get<float>(), s["color"][1].get<float>(), s["color"][2].get<float>(), s["color"][3].get<float>());
        }
        if (s.contains("bgColor") && s["bgColor"].is_array() && s["bgColor"].size() == 4) {
            style.bgColor = glm::vec4(s["bgColor"][0].get<float>(), s["bgColor"][1].get<float>(), s["bgColor"][2].get<float>(), s["bgColor"][3].get<float>());
        }
        if (s.contains("font")) style.font = s["font"].get<std::string>();
        if (s.contains("size")) style.fontSize = UIUnitValue::parseJson(s["size"]);
    }

    if (nodeJson.contains("layout")) {
        std::string l = nodeJson["layout"].get<std::string>();
        if (l == "row") YGNodeStyleSetFlexDirection(yoga, YGFlexDirectionRow);
        else if (l == "column") YGNodeStyleSetFlexDirection(yoga, YGFlexDirectionColumn);
    }

    if (nodeJson.contains("justify")) {
        std::string jst = nodeJson["justify"].get<std::string>();
        if (jst == "space-between") YGNodeStyleSetJustifyContent(yoga, YGJustifySpaceBetween);
        else if (jst == "center") YGNodeStyleSetJustifyContent(yoga, YGJustifyCenter);
        else if (jst == "space-evenly") YGNodeStyleSetJustifyContent(yoga, YGJustifySpaceEvenly);
        else if (jst == "space-around") YGNodeStyleSetJustifyContent(yoga, YGJustifySpaceAround);
        else if (jst == "flex-end") YGNodeStyleSetJustifyContent(yoga, YGJustifyFlexEnd);
        else if (jst == "flex-start") YGNodeStyleSetJustifyContent(yoga, YGJustifyFlexStart);
    }

    if (nodeJson.contains("align")) {
        std::string al = nodeJson["align"].get<std::string>();
        if (al == "center") YGNodeStyleSetAlignItems(yoga, YGAlignCenter);
        else if (al == "auto") YGNodeStyleSetAlignItems(yoga, YGAlignAuto);
        else if (al == "flex-start") YGNodeStyleSetAlignItems(yoga, YGAlignFlexStart);
        else if (al == "flex-end") YGNodeStyleSetAlignItems(yoga, YGAlignFlexEnd);
        else if (al == "stretch") YGNodeStyleSetAlignItems(yoga, YGAlignStretch);
        else if (al == "baseline") YGNodeStyleSetAlignItems(yoga, YGAlignBaseline);
    }

    if (nodeJson.contains("padding")) uiManager->applyYogaPadding(yoga, YGEdgeAll, UIUnitValue::parseJson(nodeJson["padding"]));
    if (nodeJson.contains("margin")) uiManager->applyYogaMargin(yoga, YGEdgeAll, UIUnitValue::parseJson(nodeJson["margin"]));

    if (nodeJson.contains("rotation")) rotation = nodeJson["rotation"].get<float>();

    if (nodeJson.contains("flexGrow")) YGNodeStyleSetFlexGrow(yoga, nodeJson["flexGrow"].get<float>());
    if (nodeJson.contains("flexShrink")) YGNodeStyleSetFlexShrink(yoga, nodeJson["flexShrink"].get<float>());
    if (nodeJson.contains("flexBasis")) {
        uiManager->applyYogaDimension(yoga, UIUnitValue::parseJson(nodeJson["flexBasis"]), YGNodeStyleSetFlexBasis, YGNodeStyleSetFlexBasisPercent, YGNodeStyleSetFlexBasisAuto);
    }

    if (nodeJson.contains("position")) {
        std::string pos = nodeJson["position"].get<std::string>();
        if (pos == "absolute"){
            YGNodeStyleSetPositionType(yoga, YGPositionTypeAbsolute);
            if (nodeJson.contains("left")) uiManager->applyYogaPosition(yoga, YGEdgeLeft, UIUnitValue::parseJson(nodeJson["left"]));
            if (nodeJson.contains("right")) uiManager->applyYogaPosition(yoga, YGEdgeRight, UIUnitValue::parseJson(nodeJson["right"]));
            if (nodeJson.contains("top")) uiManager->applyYogaPosition(yoga, YGEdgeTop, UIUnitValue::parseJson(nodeJson["top"]));
            if (nodeJson.contains("bottom")) uiManager->applyYogaPosition(yoga, YGEdgeBottom, UIUnitValue::parseJson(nodeJson["bottom"]));
        } else if (pos == "relative") {
            YGNodeStyleSetPositionType(yoga, YGPositionTypeRelative);
        }
    }

    if (nodeJson.contains("wrap")) {
        std::string wr = nodeJson["wrap"].get<std::string>();
        if (wr == "wrap") YGNodeStyleSetFlexWrap(yoga, YGWrapWrap);
        else if (wr == "no-wrap") YGNodeStyleSetFlexWrap(yoga, YGWrapNoWrap);
        else if (wr == "wrap-reverse") YGNodeStyleSetFlexWrap(yoga, YGWrapWrapReverse);
    }

    if (nodeJson.contains("alignSelf")) {
        std::string as = nodeJson["alignSelf"].get<std::string>();
        if (as == "center") YGNodeStyleSetAlignSelf(yoga, YGAlignCenter);
        else if (as == "auto") YGNodeStyleSetAlignSelf(yoga, YGAlignAuto);
        else if (as == "flex-start") YGNodeStyleSetAlignSelf(yoga, YGAlignFlexStart);
        else if (as == "flex-end") YGNodeStyleSetAlignSelf(yoga, YGAlignFlexEnd);
        else if (as == "stretch") YGNodeStyleSetAlignSelf(yoga, YGAlignStretch);
        else if (as == "baseline") YGNodeStyleSetAlignSelf(yoga, YGAlignBaseline);
    }

    if (nodeJson.contains("paddingLeft")) uiManager->applyYogaPadding(yoga, YGEdgeLeft, UIUnitValue::parseJson(nodeJson["paddingLeft"]));
    if (nodeJson.contains("paddingRight")) uiManager->applyYogaPadding(yoga, YGEdgeRight, UIUnitValue::parseJson(nodeJson["paddingRight"]));
    if (nodeJson.contains("paddingTop")) uiManager->applyYogaPadding(yoga, YGEdgeTop, UIUnitValue::parseJson(nodeJson["paddingTop"]));
    if (nodeJson.contains("paddingBottom")) uiManager->applyYogaPadding(yoga, YGEdgeBottom, UIUnitValue::parseJson(nodeJson["paddingBottom"]));

    if (nodeJson.contains("marginLeft")) uiManager->applyYogaMargin(yoga, YGEdgeLeft, UIUnitValue::parseJson(nodeJson["marginLeft"]));
    if (nodeJson.contains("marginRight")) uiManager->applyYogaMargin(yoga, YGEdgeRight, UIUnitValue::parseJson(nodeJson["marginRight"]));
    if (nodeJson.contains("marginTop")) uiManager->applyYogaMargin(yoga, YGEdgeTop, UIUnitValue::parseJson(nodeJson["marginTop"]));
    if (nodeJson.contains("marginBottom")) uiManager->applyYogaMargin(yoga, YGEdgeBottom, UIUnitValue::parseJson(nodeJson["marginBottom"]));

    if (nodeJson.contains("borderWidth")) {
        style.borderWidth = UIUnitValue::parseJson(nodeJson["borderWidth"]);
        YGNodeStyleSetBorder(yoga, YGEdgeAll, style.borderWidth.getPixels(uiManager));
    }

    if (nodeJson.contains("borderColor") && nodeJson["borderColor"].is_array() && nodeJson["borderColor"].size() == 4) {
        style.borderColor = glm::vec4(nodeJson["borderColor"][0].get<float>(), nodeJson["borderColor"][1].get<float>(),
                                      nodeJson["borderColor"][2].get<float>(), nodeJson["borderColor"][3].get<float>());
    }

    if (nodeJson.contains("textAlign")) {
        std::string ta = nodeJson["textAlign"].get<std::string>();
        if (ta == "center") textAlign = TextAlign::Center;
        else if (ta == "right") textAlign = TextAlign::Right;
    }
}

VexUI::VexUI(VulkanContext& ctx, VirtualFileSystem* vfs, VulkanResources* res, ResolutionManager* resMgr)
    : m_ctx(ctx), m_vfs(vfs), m_res(res), m_resMgr(resMgr) {}

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
    const size_t vbBytes = 2 * 1024 * 1024;
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bi.size = vbBytes;
    bi.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VmaAllocationCreateInfo ai{};
    ai.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    vmaCreateBuffer(m_ctx.allocator, &bi, &ai, &m_vb, &m_vbAlloc, nullptr);
    m_vbSize = vbBytes;

    VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    si.magFilter = si.minFilter = VK_FILTER_NEAREST;
    si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(m_ctx.device, &si, nullptr, &m_uiSampler);
    initialized = true;
    return true;
}

float VexUI::getPspMultiplier() const {
    if (!m_resMgr) return 1.0f;
    float current = m_resMgr->getUpscaleRatio();
    if (current <= 0.0f) return 1.0f;
    return m_resMgr->getPotencialUpscaleRatio() / current;
}

void VexUI::applyYogaDimension(YGNodeRef yoga, const UIUnitValue& val, void(*setPx)(YGNodeRef, float), void(*setPct)(YGNodeRef, float), void(*setAuto)(YGNodeRef)) const {
    if (val.type == UIUnitType::Auto) {
        if (setAuto) setAuto(yoga); else setPx(yoga, 0.f);
    } else if (val.type == UIUnitType::Percent) {
        if (setPct) setPct(yoga, val.value); else setPx(yoga, val.value);
    } else {
        setPx(yoga, val.getPixels(this));
    }
}

void VexUI::applyYogaMargin(YGNodeRef yoga, YGEdge edge, const UIUnitValue& val) const {
    if (val.type == UIUnitType::Auto) YGNodeStyleSetMarginAuto(yoga, edge);
    else if (val.type == UIUnitType::Percent) YGNodeStyleSetMarginPercent(yoga, edge, val.value);
    else YGNodeStyleSetMargin(yoga, edge, val.getPixels(this));
}

void VexUI::applyYogaPadding(YGNodeRef yoga, YGEdge edge, const UIUnitValue& val) const {
    if (val.type == UIUnitType::Percent) YGNodeStyleSetPaddingPercent(yoga, edge, val.value);
    else YGNodeStyleSetPadding(yoga, edge, val.getPixels(this));
}

void VexUI::applyYogaPosition(YGNodeRef yoga, YGEdge edge, const UIUnitValue& val) const {
    if (val.type == UIUnitType::Percent) YGNodeStyleSetPositionPercent(yoga, edge, val.value);
    else YGNodeStyleSetPosition(yoga, edge, val.getPixels(this));
}

void VexUI::safeUpdate(const std::string& id, std::function<void(Widget*)> action) {
    if (initialized) {
        if (Widget* w = findById(m_root, id)) {
            action(w);
        } else {
            log("Widget not found: %s", id.c_str());
        }
    } else {
        pendingSetters.push_back([this, id, action]() {
            if (Widget* w = findById(m_root, id)) {
                action(w);
            }
        });
    }
}

void VexUI::loadFonts(Widget* w) {
    if (!w) return;

    float pxSize = w->style.fontSize.getPixels(this);

    if (!w->style.font.empty() && pxSize > 0.f) {
        std::string key = w->style.font + "_" + std::to_string(static_cast<int>(pxSize));
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
        stbtt_GetFontVMetrics(&atlas.info, &ascent, &descent, &lineGap);

        float scale = stbtt_ScaleForPixelHeight(&atlas.info, pxSize);

        atlas.ascent = static_cast<float>(ascent) * scale;
        atlas.descent = static_cast<float>(descent) * scale;
        atlas.scale = scale;

        int texDim = 512;
        while (texDim < pxSize * 12 && texDim < 8192) {
            texDim *= 2;
        }

        const int texW = texDim, texH = texDim;
        std::vector<unsigned char> bitmap(texW * texH, 0);
        atlas.cdata.resize(96);
        stbtt_BakeFontBitmap((unsigned char*)data->data.data(), 0, pxSize, bitmap.data(), texW, texH, 32, 96, atlas.cdata.data());

        std::vector<unsigned char> rgba(texW * texH * 4);
        for (int i = 0; i < texW * texH; ++i) {
            unsigned char v = bitmap[i];
            v = (v > 127) ? 255 : 0;
            rgba[i*4 + 0] = 255;
            rgba[i*4 + 1] = 255;
            rgba[i*4 + 2] = 255;
            rgba[i*4 + 3] = v;
        }

        std::string texName = "ui_font_" + key;
        m_res->createTextureFromRaw(rgba, texW, texH, texName);

        atlas.texIdx = m_res->getTextureIndex(texName);
        atlas.width = texW;
        atlas.height = texH;
        atlas.bakedSize = pxSize;
        m_fontAtlases[key] = atlas;
    }

recurse:
    const auto& children = w->children;
    size_t count = children.size();

    for (size_t i = 0; i < count; ++i) {
        if (i + 1 < count) {
            Widget* nextSibling = children[i + 1];
            _mm_prefetch(reinterpret_cast<const char*>(nextSibling) + 64, _MM_HINT_T0);
        }
        loadFonts(children[i]);
    }
}

void VexUI::loadImages(Widget* w) {
    if (!w) return;
    if (!w->image.empty()) {
        m_res->loadTexture(GetAssetPath(w->image), GetAssetPath(w->image));
    }

    const auto& children = w->children;
    size_t count = children.size();

    for (size_t i = 0; i < count; ++i) {
        if (i + 1 < count) {
            Widget* nextSibling = children[i + 1];
            _mm_prefetch(reinterpret_cast<const char*>(nextSibling) + 64, _MM_HINT_T0);
        }
        loadImages(children[i]);
    }
}

Widget* VexUI::parseNode(const nlohmann::json& j) {
    Widget* w = new Widget();
    w->ui = this;
    w->yoga = YGNodeNew();
    YGNodeSetContext(w->yoga, w);

    w->nodeJson = j;
    w->applyLayout(this);

    if (j.contains("children") && j["children"].is_array()) {
        for (const auto& c : j["children"]) {
            Widget* child = parseNode(c);
            child->parent = w;
            auto childYoga = child->yoga;

            if ((child->type == WidgetType::Label || child->type == WidgetType::Button)) {
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
        m_focusedWidget = nullptr;
        if (json.contains("root")) {
            m_root = parseNode(json["root"]);
            if (json["root"].contains("zindex")){
                zIndex = json["root"]["zindex"].get<int>();
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

Widget* VexUI::findWidgetAt(Widget* w, glm::vec2 pos, glm::vec2 parentOffset) {
    if (!w) return nullptr;

    float absX = parentOffset.x + YGNodeLayoutGetLeft(w->yoga);
    float absY = parentOffset.y + YGNodeLayoutGetTop(w->yoga);
    float width  = YGNodeLayoutGetWidth(w->yoga);
    float height = YGNodeLayoutGetHeight(w->yoga);

    glm::vec2 pivot = { absX + width * 0.5f, absY + height * 0.5f };

    glm::vec2 checkPos = pos;
    if (w->rotation != 0.f) {
        float rads = glm::radians(-w->rotation);
        float c = cos(rads);
        float s = sin(rads);

        glm::vec2 d = pos - pivot;
        checkPos.x = pivot.x + (d.x * c - d.y * s);
        checkPos.y = pivot.y + (d.x * s + d.y * c);
    }

    bool isInside = (checkPos.x >= absX && checkPos.x <= absX + width &&
                     checkPos.y >= absY && checkPos.y <= absY + height);

    for (auto it = w->children.rbegin(); it != w->children.rend(); ++it) {
        if (Widget* hit = findWidgetAt(*it, pos, {absX, absY})) {
            return hit;
        }
    }

    return isInside ? w : nullptr;
}

Widget* VexUI::findById(Widget* w, const std::string& id) {
    if (!w) return nullptr;
    if (w->id == id) return w;
    for (auto* c : w->children) if (auto* f = findById(c, id)) return f;
    return nullptr;
}

void VexUI::setText(const std::string& id, const std::string& txt) {
    safeUpdate(id, [txt](Widget* w) {
        if (w->text != txt) {
            w->text = txt;
            w->nodeJson["text"] = txt;
            if (w->yoga && YGNodeHasMeasureFunc(w->yoga)) {
                YGNodeMarkDirty(w->yoga);
            }
        }
    });
}

void VexUI::setOnClick(const std::string& id, std::function<void()> cb) {
    safeUpdate(id, [cb](Widget* w) {
        if (w->type == WidgetType::Button) w->onClick = std::move(cb);
    });
}

void VexUI::processEvent(const SDL_Event& ev) {
    if (!m_root) return;
    float sx = static_cast<float>(m_ctx.swapchainExtent.width)  / m_ctx.currentRenderResolution.x;
    float sy = static_cast<float>(m_ctx.swapchainExtent.height) / m_ctx.currentRenderResolution.y;

    if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        log("Mouse button down. pos: (%f, %f)", ev.button.x / sx, ev.button.y / sy);
        glm::vec2 mouse(ev.button.x / sx, ev.button.y / sy);
        if (Widget* w = findWidgetAt(m_root, mouse, {0, 0}); w && w->type == WidgetType::Button && w->onClick)
            w->onClick();
    }
    else if (ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        SDL_GamepadButton button = (SDL_GamepadButton)ev.gbutton.button;

        if (button == SDL_GAMEPAD_BUTTON_SOUTH) {
            if (m_focusedWidget && m_focusedWidget->onClick) {
                m_focusedWidget->onClick();
            }
        }
        else if (button == SDL_GAMEPAD_BUTTON_DPAD_LEFT) navigateToWidget(-1.0f, 0.0f);
        else if (button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) navigateToWidget(1.0f, 0.0f);
        else if (button == SDL_GAMEPAD_BUTTON_DPAD_UP) navigateToWidget(0.0f, -1.0f);
        else if (button == SDL_GAMEPAD_BUTTON_DPAD_DOWN) navigateToWidget(0.0f, 1.0f);
    }
    else if (ev.type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
        SDL_GamepadAxis axis = (SDL_GamepadAxis)ev.gaxis.axis;
        float value = ev.gaxis.value / 32767.0f;

        if (axis == SDL_GAMEPAD_AXIS_LEFTX) {
            bool wasAbove = std::abs(m_gamepadAxisX) > GAMEPAD_AXIS_THRESHOLD;
            bool isAbove = std::abs(value) > GAMEPAD_AXIS_THRESHOLD;

            if (!wasAbove && isAbove) {
                float dirX = (value > 0.0f) ? 1.0f : -1.0f;
                navigateToWidget(dirX, 0.0f);
            }
            m_gamepadAxisX = value;
        }
        else if (axis == SDL_GAMEPAD_AXIS_LEFTY) {
            bool wasAbove = std::abs(m_gamepadAxisY) > GAMEPAD_AXIS_THRESHOLD;
            bool isAbove = std::abs(value) > GAMEPAD_AXIS_THRESHOLD;

            if (!wasAbove && isAbove) {
                float dirY = (value > 0.0f) ? 1.0f : -1.0f;
                navigateToWidget(0.0f, dirY);
            }
            m_gamepadAxisY = value;
        }
    }
}
void VexUI::navigateToWidget(float dirX, float dirY) {
    std::vector<Widget*> navigable;
    getNavigableWidgets(navigable);

    if (navigable.empty()) return;

    bool focusedIsValid = false;
    if (m_focusedWidget) {
        for (Widget* w : navigable) {
            if (w == m_focusedWidget) { focusedIsValid = true; break; }
        }
    }

    if (!focusedIsValid) {
        setFocusedWidget(navigable.front());
        return;
    }

    glm::vec2 fromPos = getWidgetCenter(m_focusedWidget);
    glm::vec2 direction{dirX, dirY};

    Widget* nextWidget = findClosestNavigableWidget(fromPos, direction, navigable);
    if (nextWidget) setFocusedWidget(nextWidget);
}

void VexUI::getNavigableWidgets(std::vector<Widget*>& out) { if (m_root) collectNavigableWidgets(m_root, out); }

void VexUI::collectNavigableWidgets(Widget* w, std::vector<Widget*>& out) {
    if (!w) return;
    if (isWidgetNavigable(w)) out.push_back(w);
    for (Widget* child : w->children) collectNavigableWidgets(child, out);
}

bool VexUI::isWidgetNavigable(Widget* w) const {
    if (!w) return false;
    return w->type == WidgetType::Button;
}

glm::vec2 VexUI::getWidgetCenter(Widget* w) {
    if (!w || !w->yoga) return {0.0f, 0.0f};

    float x = YGNodeLayoutGetLeft(w->yoga);
    float y = YGNodeLayoutGetTop(w->yoga);
    float width = YGNodeLayoutGetWidth(w->yoga);
    float height = YGNodeLayoutGetHeight(w->yoga);

    Widget* parent = w->parent;
    while (parent) {
        x += YGNodeLayoutGetLeft(parent->yoga);
        y += YGNodeLayoutGetTop(parent->yoga);
        parent = parent->parent;
    }
    return {x + width * 0.5f, y + height * 0.5f};
}

Widget* VexUI::findClosestNavigableWidget(const glm::vec2& fromPos, const glm::vec2& direction, const std::vector<Widget*>& candidates) {
    Widget* closest = nullptr;
    float closestScore = -FLT_MAX;
    bool isHorizontalNav = std::abs(direction.x) > std::abs(direction.y);

    for (Widget* candidate : candidates) {
        if (candidate == m_focusedWidget) continue;

        glm::vec2 toCandidate = getWidgetCenter(candidate) - fromPos;
        float distance = glm::length(toCandidate);
        if (distance < 1.0f) continue;

        float dot = glm::dot(glm::normalize(direction), glm::normalize(toCandidate));
        float crossAlignment = 1.0f;

        if (isHorizontalNav) {
            crossAlignment = std::max(0.0f, 1.0f - (std::abs(toCandidate.y) / 500.0f));
        } else {
            crossAlignment = std::max(0.0f, 1.0f - (std::abs(toCandidate.x) / 500.0f));
        }

        if (dot > -0.2f) {
            float score = dot * 3.0f + crossAlignment * 0.5f - (distance / 800.0f);
            if (score > closestScore) {
                closestScore = score;
                closest = candidate;
            }
        }
    }
    return closest;
}

void VexUI::setFocusedWidget(Widget* w) {
    if (w && !isWidgetNavigable(w)) return;
    if (w == m_focusedWidget) return;

    if (m_focusedWidget && m_focusedWidget->onFocusLost) m_focusedWidget->onFocusLost();
    m_focusedWidget = w;
    if (m_focusedWidget && m_focusedWidget->onFocusEnter) m_focusedWidget->onFocusEnter();
}

YGSize VexUI::calculateTextSize(Widget* w, float maxWidth) {
    float pxSize = w->style.fontSize.getPixels(this);
    std::string key = w->style.font + "_" + std::to_string(static_cast<int>(pxSize));
    auto it = m_fontAtlases.find(key);
    if (it == m_fontAtlases.end() || w->text.empty()) return {0, 0};

    const FontAtlas& a = it->second;

    std::vector<std::string> lines = wrapText(w->text, a, maxWidth);

    float lineHeight = a.ascent - a.descent;
    float totalHeight = lines.size() * lineHeight;
    float totalWidth = 0.f;

    for (const auto& line : lines) {
        float lineWidth = 0.f;
        for (char ch : line) {
            if (ch < 32 || ch > 127) continue;
            lineWidth += a.cdata[ch - 32].xadvance;
        }
        totalWidth = std::max(totalWidth, lineWidth);
    }

    return {totalWidth, totalHeight};
}

YGSize VexUI::measureTextNode(const YGNode* node, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
    Widget* w = static_cast<Widget*>(YGNodeGetContext(node));
    if (!w || !w->ui) return {0, 0};

    float maxW = FLT_MAX;
    if (widthMode == YGMeasureModeExactly || widthMode == YGMeasureModeAtMost) maxW = width;

    YGSize naturalSize = w->ui->calculateTextSize(w, maxW);
    YGSize measuredSize;

    if (widthMode == YGMeasureModeExactly) measuredSize.width = width;
    else if (widthMode == YGMeasureModeAtMost) measuredSize.width = std::min(width, naturalSize.width);
    else measuredSize.width = naturalSize.width;

    if (heightMode == YGMeasureModeExactly) measuredSize.height = height;
    else if (heightMode == YGMeasureModeAtMost) measuredSize.height = std::min(height, naturalSize.height);
    else measuredSize.height = naturalSize.height;

    return measuredSize;
}

void VexUI::batch(Widget* w, std::vector<float>& verts, glm::vec2 parentOffset) {
    if (!w) return;

    float x = parentOffset.x + YGNodeLayoutGetLeft(w->yoga);
    float y = parentOffset.y + YGNodeLayoutGetTop(w->yoga);

    float width  = YGNodeLayoutGetWidth(w->yoga);
    float height = YGNodeLayoutGetHeight(w->yoga);

    glm::vec2 pivot = { x + width * 0.5f, y + height * 0.5f };

    float rads = glm::radians(w->rotation);
    float cosA = cos(rads);
    float sinA = sin(rads);

    auto pushQuad = [&](float x0, float y0, float u0, float v0,
                        float x1, float y1, float u1, float v1,
                        const glm::vec4& col, float texIdx) {
        float rV[8];

        static bool useAVX2 = HardwareInfo::HasAVX2();

        if (useAVX2) rotateQuadAvX2(rV, pivot.x, pivot.y, sinA, cosA, x0, y0, x1, y1);
        else rotateQuadScalar(rV, pivot.x, pivot.y, sinA, cosA, x0, y0, x1, y1);

        verts.insert(verts.end(), {rV[0], rV[1], u0, v0, col.r, col.g, col.b, col.a, texIdx});
        verts.insert(verts.end(), {rV[2], rV[3], u1, v0, col.r, col.g, col.b, col.a, texIdx});
        verts.insert(verts.end(), {rV[4], rV[5], u0, v1, col.r, col.g, col.b, col.a, texIdx});

        verts.insert(verts.end(), {rV[2], rV[3], u1, v0, col.r, col.g, col.b, col.a, texIdx});
        verts.insert(verts.end(), {rV[6], rV[7], u1, v1, col.r, col.g, col.b, col.a, texIdx});
        verts.insert(verts.end(), {rV[4], rV[5], u0, v1, col.r, col.g, col.b, col.a, texIdx});
    };

    if (w->style.bgColor.a > 0.f) {
        pushQuad(x, y, 0, 0, x + width, y + height, 1, 1, w->style.bgColor, -1.f);
    }

    float bw = w->style.borderWidth.getPixels(this);
    if (bw > 0.f && w->style.borderColor.a > 0.f) {
        pushQuad(x, y, 0, 0, x + width, y + bw, 1, 1, w->style.borderColor, -1.f);
        pushQuad(x, y + height - bw, 0, 0, x + width, y + height, 1, 1, w->style.borderColor, -1.f);
        pushQuad(x, y, 0, 0, x + bw, y + height, 1, 1, w->style.borderColor, -1.f);
        pushQuad(x + width - bw, y, 0, 0, x + width, y + height, 1, 1, w->style.borderColor, -1.f);
    }

    if ((w->type == WidgetType::Label || w->type == WidgetType::Button) && !w->text.empty() && !w->style.font.empty()) {
        float pxSize = w->style.fontSize.getPixels(this);
        std::string key = w->style.font + "_" + std::to_string(static_cast<int>(pxSize));
        auto it = m_fontAtlases.find(key);
        if (it != m_fontAtlases.end()) {
            const FontAtlas& a = it->second;

            std::vector<std::string> lines = wrapText(w->text, a, width);

            float cy = y + a.ascent;

            for (const auto& line : lines) {
                float lineWidth = 0.f;
                for (char ch : line) {
                    if (ch < 32 || ch > 127) continue;
                    lineWidth += a.cdata[ch - 32].xadvance;
                }

                float startX = x;
                if (w->textAlign == TextAlign::Center) startX = x + (width - lineWidth) / 2.f;
                else if (w->textAlign == TextAlign::Right) startX = x + (width - lineWidth);

                float cx = startX;
                for (char ch : line) {
                    if (ch < 32 || ch > 127) continue;
                    const stbtt_bakedchar& cd = a.cdata[ch - 32];

                    pushQuad(cx + cd.xoff, cy + cd.yoff, cd.x0 / float(a.width), cd.y0 / float(a.height),
                             cx + cd.xoff + (cd.x1 - cd.x0), cy + cd.yoff + (cd.y1 - cd.y0),
                             cd.x1 / float(a.width), cd.y1 / float(a.height),
                             w->style.color, static_cast<float>(a.texIdx));
                    cx += cd.xadvance;
                }
                cy += (a.ascent - a.descent);
            }
        }
    }

    if (w->type == WidgetType::Image && !w->image.empty()) {
        uint32_t idx = m_res->getTextureIndex(GetAssetPath(w->image));
        if (idx != UINT32_MAX) pushQuad(x, y, 0, 0, x + width, y + height, 1, 1, {1,1,1,1}, static_cast<float>(idx));
    }

    const auto& children = w->children;
    size_t count = children.size();

    for (size_t i = 0; i < count; ++i) {
        if (i + 1 < count) {
            Widget* nextSibling = children[i + 1];
            _mm_prefetch(reinterpret_cast<const char*>(nextSibling) + 64, _MM_HINT_T0);
        }
        batch(children[i], verts, {x, y});
    }
}

void VexUI::render(VkCommandBuffer cmd, VkPipeline pipeline, VkPipelineLayout pipelineLayout, int currentFrame) {
    if (!m_root) return;

    glm::uvec2 currentRes = m_ctx.currentRenderResolution;
    float currentPspMult = getPspMultiplier();

    if (m_lastRenderRes != currentRes || m_lastPspMult != currentPspMult) {
        m_lastRenderRes = currentRes;
        m_lastPspMult = currentPspMult;

        std::function<void(Widget*)> syncTree = [&](Widget* w) {
            if (!w) return;
            w->applyLayout(this);
            for (auto* c : w->children) syncTree(c);
        };
        syncTree(m_root);

        for (auto& [k, a] : m_fontAtlases) {
            if (a.view) vkDestroyImageView(m_ctx.device, a.view, nullptr);
            if (a.image) vmaDestroyImage(m_ctx.allocator, a.image, a.alloc);
        }
        m_fontAtlases.clear();
        loadFonts(m_root);
    }

    layout(m_ctx.currentRenderResolution);
    vkDeviceWaitIdle(m_ctx.device);

    std::vector<float> verts;
    verts.reserve(1024 * 9);
    batch(m_root, verts);
    uploadVerts(verts);

    if (verts.empty()) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    if (VkDescriptorSet globalUBO = m_res->getUBODescriptorSet(currentFrame); globalUBO != VK_NULL_HANDLE) {
        uint32_t dynamicOffset = 0;
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &globalUBO, 1, &dynamicOffset);
    }

    UIPushConstants uiPC{ glm::ortho(0.0f, static_cast<float>(m_ctx.currentRenderResolution.x), 0.0f, static_cast<float>(m_ctx.currentRenderResolution.y), -1.0f, 1.0f) };
    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UIPushConstants), &uiPC);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_vb, &offset);

    uint32_t totalVertices = verts.size() / 9;
    uint32_t currentVertex = 0;
    int currentTexIndex = INT_MIN;

    while (currentVertex < totalVertices) {
        int texIndex = static_cast<int>(verts[currentVertex * 9 + 8]);
        if (texIndex != currentTexIndex) {
            currentTexIndex = texIndex;
            if (!m_ctx.supportsBindlessTextures) {
                VkDescriptorSet currentTexSet = m_res->getTextureDescriptorSet(currentFrame, texIndex >= 0 ? texIndex : 0);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &currentTexSet, 0, nullptr);
            }
        }
        vkCmdDraw(cmd, 6, 1, currentVertex, 0);
        currentVertex += 6;
    }
}

void VexUI::uploadVerts(const std::vector<float>& verts) {
    if (verts.empty()) return;
    size_t bytes = verts.size() * sizeof(float);
    void* dst; vmaMapMemory(m_ctx.allocator, m_vbAlloc, &dst);
    memcpy(dst, verts.data(), bytes);
    vmaUnmapMemory(m_ctx.allocator, m_vbAlloc);
}

void VexUI::freeTree(Widget* w) {
    if (!w) return;
    for (auto* c : w->children) freeTree(c);
    delete w;
}

void VexUI::setRotation(const std::string& id, float degrees) {
    safeUpdate(id, [this, degrees](Widget* w) {
        w->nodeJson["rotation"] = degrees;
        w->applyLayout(this);
    });
}

void VexUI::setPosition(const std::string& id, UIUnitValue x, UIUnitValue y) {
    safeUpdate(id, [this, x, y](Widget* w) {
        w->nodeJson["position"] = "absolute";
        w->nodeJson["left"] = x.toJson();
        w->nodeJson["top"] = y.toJson();
        w->applyLayout(this);
    });
}

void VexUI::setSize(const std::string& id, UIUnitValue width, UIUnitValue height) {
    safeUpdate(id, [this, width, height](Widget* w) {
        w->nodeJson["size"] = {width.toJson(), height.toJson()};
        w->applyLayout(this);
    });
}

void VexUI::setImage(const std::string& id, const std::string& path) {
    safeUpdate(id, [this, path](Widget* w) {
        w->image = path;
        w->nodeJson["image"] = path;
        m_res->loadTexture(GetAssetPath(path), GetAssetPath(path));
    });
}

void VexUI::setFont(const std::string& id, const std::string& fontPath, UIUnitValue fontSize) {
    safeUpdate(id, [this, fontPath, fontSize](Widget* w) {
        w->nodeJson["style"]["font"] = fontPath;
        w->nodeJson["style"]["size"] = fontSize.toJson();
        w->applyLayout(this);
        this->loadFonts(w);
        if (w->yoga && YGNodeHasMeasureFunc(w->yoga)) YGNodeMarkDirty(w->yoga);
    });
}

void VexUI::setColor(const std::string& id, glm::vec4 color) {
    safeUpdate(id, [this, color](Widget* w) {
        w->nodeJson["style"]["color"] = {color.r, color.g, color.b, color.a};
        w->applyLayout(this);
    });
}

void VexUI::setBackgroundColor(const std::string& id, glm::vec4 color) {
    safeUpdate(id, [this, color](Widget* w) {
        w->nodeJson["style"]["bgColor"] = {color.r, color.g, color.b, color.a};
        w->applyLayout(this);
    });
}

void VexUI::setBorder(const std::string& id, UIUnitValue width, glm::vec4 color) {
    safeUpdate(id, [this, width, color](Widget* w) {
        w->nodeJson["borderWidth"] = width.toJson();
        w->nodeJson["borderColor"] = {color.r, color.g, color.b, color.a};
        w->applyLayout(this);
    });
}

} // namespace vex
