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

void calculateLayout(LayoutNode* node, float availableW, float availableH, bool isRoot, const VexUI* ui, float forcedW, float forcedH) {
    if (!node) return;

    float pl = node->paddingLeft.resolveOr(availableW, 0.f);
    float pr = node->paddingRight.resolveOr(availableW, 0.f);
    float pt = node->paddingTop.resolveOr(availableH, 0.f);
    float pb = node->paddingBottom.resolveOr(availableH, 0.f);
    float bw = node->borderWidth.resolveOr(availableW, 0.f);
    float padX = pl + pr + bw * 2.f;
    float padY = pt + pb + bw * 2.f;

    float explicitW = !std::isnan(forcedW) ? forcedW : node->width.resolve(availableW);
    float explicitH = !std::isnan(forcedH) ? forcedH : node->height.resolve(availableH);

    if (isRoot) {
        explicitW = availableW;
        explicitH = availableH;
    }

    float innerAvailW = std::isnan(explicitW) ? NAN : std::max(0.0f, explicitW - padX);
    float innerAvailH = std::isnan(explicitH) ? NAN : std::max(0.0f, explicitH - padY);

    if (node->measureFunc) {
        float maxW = std::isnan(innerAvailW) ? FLT_MAX : innerAvailW;
        float maxH = std::isnan(innerAvailH) ? FLT_MAX : innerAvailH;

        node->measureFunc(node, maxW, maxH);

        float measuredW = node->computedWidth + padX;
        float measuredH = node->computedHeight + padY;

        node->computedWidth = std::isnan(explicitW) ? measuredW : explicitW;
        node->computedHeight = std::isnan(explicitH) ? measuredH : explicitH;
        return;
    }

    std::vector<LayoutNode*> flow;
    for (auto* c : node->children) {
        if (c->positionType == PositionType::Absolute) continue;
        flow.push_back(c);
    }

    float totalMain = 0.f;
    float maxCross = 0.f;
    float totalGrow = 0.f;
    float totalShrink = 0.f;

    for (auto* c : flow) {
        calculateLayout(c, innerAvailW, innerAvailH, false, ui);

        float cml = c->marginLeft.resolveOr(innerAvailW, 0.f);
        float cmr = c->marginRight.resolveOr(innerAvailW, 0.f);
        float cmt = c->marginTop.resolveOr(innerAvailH, 0.f);
        float cmb = c->marginBottom.resolveOr(innerAvailH, 0.f);

        float outerW = c->computedWidth + cml + cmr;
        float outerH = c->computedHeight + cmt + cmb;

        if (node->flexDirection == FlexDirection::Row) {
            totalMain += outerW;
            maxCross = std::max(maxCross, outerH);
        } else {
            totalMain += outerH;
            maxCross = std::max(maxCross, outerW);
        }
        totalGrow += c->flexGrow;
        totalShrink += c->flexShrink;
    }

    node->computedWidth = !std::isnan(explicitW) ? explicitW : (node->flexDirection == FlexDirection::Row ? totalMain : maxCross) + padX;
    node->computedHeight = !std::isnan(explicitH) ? explicitH : (node->flexDirection == FlexDirection::Column ? totalMain : maxCross) + padY;

    float innerW = std::max(0.0f, node->computedWidth - padX);
    float innerH = std::max(0.0f, node->computedHeight - padY);
    float freeMain = (node->flexDirection == FlexDirection::Row ? innerW : innerH) - totalMain;

    bool needsRelayout = false;

    for (auto* c : flow) {
        float childForcedW = NAN;
        float childForcedH = NAN;
        bool changed = false;

        if (freeMain > 0.f && totalGrow > 0.f && c->flexGrow > 0.f) {
            float extra = freeMain * (c->flexGrow / totalGrow);
            if (node->flexDirection == FlexDirection::Row) childForcedW = c->computedWidth + extra;
            else childForcedH = c->computedHeight + extra;
            changed = true;
        } else if (freeMain < 0.f && totalShrink > 0.f && c->flexShrink > 0.f) {
            float shrink = (-freeMain) * (c->flexShrink / totalShrink);
            if (node->flexDirection == FlexDirection::Row) childForcedW = std::max(0.0f, c->computedWidth - shrink);
            else childForcedH = std::max(0.0f, c->computedHeight - shrink);
            changed = true;
        }

        Align al = c->alignSelf == Align::Auto ? node->alignItems : c->alignSelf;
        if (al == Align::Stretch) {
            float cml = c->marginLeft.resolveOr(innerW, 0.f);
            float cmr = c->marginRight.resolveOr(innerW, 0.f);
            float cmt = c->marginTop.resolveOr(innerH, 0.f);
            float cmb = c->marginBottom.resolveOr(innerH, 0.f);

            if (node->flexDirection == FlexDirection::Row && std::isnan(c->height.resolve(innerH))) {
                childForcedH = std::max(0.0f, innerH - cmt - cmb);
                changed = true;
            } else if (node->flexDirection == FlexDirection::Column && std::isnan(c->width.resolve(innerW))) {
                childForcedW = std::max(0.0f, innerW - cml - cmr);
                changed = true;
            }
        }

        if (changed) {
            calculateLayout(c, innerW, innerH, false, ui, childForcedW, childForcedH);
            needsRelayout = true;
        }
    }

    if (needsRelayout) {
        totalMain = 0.f;
        for (auto* c : flow) {
            float cml = c->marginLeft.resolveOr(innerW, 0.f);
            float cmr = c->marginRight.resolveOr(innerW, 0.f);
            float cmt = c->marginTop.resolveOr(innerH, 0.f);
            float cmb = c->marginBottom.resolveOr(innerH, 0.f);
            totalMain += (node->flexDirection == FlexDirection::Row) ? (c->computedWidth + cml + cmr) : (c->computedHeight + cmt + cmb);
        }
        freeMain = (node->flexDirection == FlexDirection::Row ? innerW : innerH) - totalMain;
    }

    float mainPos = node->flexDirection == FlexDirection::Row ? (pl + bw) : (pt + bw);
    float crossStart = node->flexDirection == FlexDirection::Row ? (pt + bw) : (pl + bw);
    float space = 0.f;

    if (freeMain > 0.f && totalGrow == 0.f) {
        if (node->justifyContent == Justify::Center) mainPos += freeMain / 2.0f;
        else if (node->justifyContent == Justify::FlexEnd) mainPos += freeMain;
        else if (node->justifyContent == Justify::SpaceBetween && flow.size() > 1) {
            space = freeMain / (flow.size() - 1);
        }
    }

    for (auto* c : flow) {
        float cml = c->marginLeft.resolveOr(innerW, 0.f);
        float cmr = c->marginRight.resolveOr(innerW, 0.f);
        float cmt = c->marginTop.resolveOr(innerH, 0.f);
        float cmb = c->marginBottom.resolveOr(innerH, 0.f);

        float cMain = node->flexDirection == FlexDirection::Row ? (c->computedWidth + cml + cmr) : (c->computedHeight + cmt + cmb);
        float cCross = node->flexDirection == FlexDirection::Row ? (c->computedHeight + cmt + cmb) : (c->computedWidth + cml + cmr);
        float pCross = node->flexDirection == FlexDirection::Row ? innerH : innerW;

        float crossOff = 0.f;
        Align al = c->alignSelf == Align::Auto ? node->alignItems : c->alignSelf;
        if (al == Align::Baseline) al = Align::FlexStart;

        if (al == Align::Center) crossOff = (pCross - cCross) / 2.0f;
        else if (al == Align::FlexEnd) crossOff = pCross - cCross;

        if (node->flexDirection == FlexDirection::Row) {
            c->computedLeft = mainPos + cml;
            c->computedTop = crossStart + crossOff + cmt;
        } else {
            c->computedTop = mainPos + cmt;
            c->computedLeft = crossStart + crossOff + cml;
        }
        mainPos += cMain + space;
    }

    for (auto* c : node->children) {
        if (c->positionType != PositionType::Absolute) continue;

        calculateLayout(c, innerW, innerH, false, ui);

        float cml = c->marginLeft.resolveOr(innerW, 0.f);
        float cmr = c->marginRight.resolveOr(innerW, 0.f);
        float cmt = c->marginTop.resolveOr(innerH, 0.f);
        float cmb = c->marginBottom.resolveOr(innerH, 0.f);

        float l = c->left.resolve(innerW);
        float r = c->right.resolve(innerW);
        float t = c->top.resolve(innerH);
        float b = c->bottom.resolve(innerH);

        if (!std::isnan(l)) c->computedLeft = bw + l + cml;
        else if (!std::isnan(r)) c->computedLeft = node->computedWidth - bw - c->computedWidth - r - cmr;
        else c->computedLeft = bw + cml;

        if (!std::isnan(t)) c->computedTop = bw + t + cmt;
        else if (!std::isnan(b)) c->computedTop = node->computedHeight - bw - c->computedHeight - b - cmb;
        else c->computedTop = bw + cmt;
    }
}

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
                for (char r : currentLine) currentWidth += a.cdata[r - 32].xadvance;
                lastSpacePos = std::string::npos;
            } else {
                lines.push_back(currentLine);
                currentLine = std::string(1, ch);
                currentWidth = advance;
                lastSpacePos = std::string::npos;
            }
        } else {
            if (ch == ' ') lastSpacePos = currentLine.length();
            currentLine += ch;
            currentWidth += advance;
        }
    }
    if (!currentLine.empty()) lines.push_back(currentLine);
    return lines;
}

static void rotateQuadScalar(float* outVerts, float pivotX, float pivotY, float sinA, float cosA, float x0, float y0, float x1, float y1) {
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
static void rotateQuadAvX2(float* outVerts, float pivotX, float pivotY, float sinA, float cosA, float x0, float y0, float x1, float y1) {
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
        case UIUnitType::Auto: return NAN;
    }
    return NAN;
}

Widget::~Widget() {
    if (layoutNode) delete layoutNode;
}

void Widget::applyLayout(VexUI* uiManager) {
    if (!uiManager || !layoutNode) return;

    auto mapUnit = [&](const UIUnitValue& val) -> FlexValue {
        if (val.type == UIUnitType::Auto) return {NAN, false};
        if (val.type == UIUnitType::Percent) return {val.value, true};
        return {val.getPixels(uiManager), false};
    };

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
        layoutNode->width = mapUnit(size.x);
        layoutNode->height = mapUnit(size.y);
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
        if (l == "row") layoutNode->flexDirection = FlexDirection::Row;
        else if (l == "column") layoutNode->flexDirection = FlexDirection::Column;
    }

    if (nodeJson.contains("justify")) {
        std::string jst = nodeJson["justify"].get<std::string>();
        if (jst == "space-between") layoutNode->justifyContent = Justify::SpaceBetween;
        else if (jst == "center") layoutNode->justifyContent = Justify::Center;
        else if (jst == "flex-end") layoutNode->justifyContent = Justify::FlexEnd;
        else if (jst == "flex-start") layoutNode->justifyContent = Justify::FlexStart;
    }

    if (nodeJson.contains("align")) {
        std::string al = nodeJson["align"].get<std::string>();
        if (al == "baseline") layoutNode->alignItems = Align::Baseline;
        else if (al == "center") layoutNode->alignItems = Align::Center;
        else if (al == "flex-start") layoutNode->alignItems = Align::FlexStart;
        else if (al == "flex-end") layoutNode->alignItems = Align::FlexEnd;
        else if (al == "stretch") layoutNode->alignItems = Align::Stretch;
    }

    if (nodeJson.contains("rotation")) rotation = nodeJson["rotation"].get<float>();
    if (nodeJson.contains("flexGrow")) layoutNode->flexGrow = nodeJson["flexGrow"].get<float>();
    if (nodeJson.contains("flexShrink")) layoutNode->flexShrink = nodeJson["flexShrink"].get<float>();

    if (nodeJson.contains("position")) {
        std::string pos = nodeJson["position"].get<std::string>();
        if (pos == "absolute") {
            layoutNode->positionType = PositionType::Absolute;
            if (nodeJson.contains("left")) layoutNode->left = mapUnit(UIUnitValue::parseJson(nodeJson["left"]));
            if (nodeJson.contains("right")) layoutNode->right = mapUnit(UIUnitValue::parseJson(nodeJson["right"]));
            if (nodeJson.contains("top")) layoutNode->top = mapUnit(UIUnitValue::parseJson(nodeJson["top"]));
            if (nodeJson.contains("bottom")) layoutNode->bottom = mapUnit(UIUnitValue::parseJson(nodeJson["bottom"]));
        } else if (pos == "relative") {
            layoutNode->positionType = PositionType::Relative;
        }
    }

    if (nodeJson.contains("padding")) {
        auto p = mapUnit(UIUnitValue::parseJson(nodeJson["padding"]));
        layoutNode->paddingLeft = layoutNode->paddingRight = layoutNode->paddingTop = layoutNode->paddingBottom = p;
    } else if (nodeJson.contains("pading")) {
        auto p = mapUnit(UIUnitValue::parseJson(nodeJson["pading"]));
        layoutNode->paddingLeft = layoutNode->paddingRight = layoutNode->paddingTop = layoutNode->paddingBottom = p;
    }

    if (nodeJson.contains("paddingLeft")) layoutNode->paddingLeft = mapUnit(UIUnitValue::parseJson(nodeJson["paddingLeft"]));
    if (nodeJson.contains("paddingRight")) layoutNode->paddingRight = mapUnit(UIUnitValue::parseJson(nodeJson["paddingRight"]));
    if (nodeJson.contains("paddingTop")) layoutNode->paddingTop = mapUnit(UIUnitValue::parseJson(nodeJson["paddingTop"]));
    if (nodeJson.contains("paddingBottom")) layoutNode->paddingBottom = mapUnit(UIUnitValue::parseJson(nodeJson["paddingBottom"]));

    if (nodeJson.contains("margin")) {
        auto m = mapUnit(UIUnitValue::parseJson(nodeJson["margin"]));
        layoutNode->marginLeft = layoutNode->marginRight = layoutNode->marginTop = layoutNode->marginBottom = m;
    }
    if (nodeJson.contains("marginLeft")) layoutNode->marginLeft = mapUnit(UIUnitValue::parseJson(nodeJson["marginLeft"]));
    if (nodeJson.contains("marginRight")) layoutNode->marginRight = mapUnit(UIUnitValue::parseJson(nodeJson["marginRight"]));
    if (nodeJson.contains("marginTop")) layoutNode->marginTop = mapUnit(UIUnitValue::parseJson(nodeJson["marginTop"]));
    if (nodeJson.contains("marginBottom")) layoutNode->marginBottom = mapUnit(UIUnitValue::parseJson(nodeJson["marginBottom"]));

    if (nodeJson.contains("borderWidth")) {
        style.borderWidth = UIUnitValue::parseJson(nodeJson["borderWidth"]);
        layoutNode->borderWidth = mapUnit(style.borderWidth);
    }
    if (nodeJson.contains("borderColor") && nodeJson["borderColor"].is_array() && nodeJson["borderColor"].size() == 4) {
        style.borderColor = glm::vec4(nodeJson["borderColor"][0].get<float>(), nodeJson["borderColor"][1].get<float>(), nodeJson["borderColor"][2].get<float>(), nodeJson["borderColor"][3].get<float>());
    }

    if (nodeJson.contains("textAlign")) {
        std::string ta = nodeJson["textAlign"].get<std::string>();
        if (ta == "center") textAlign = TextAlign::Center;
        else if (ta == "right") textAlign = TextAlign::Right;
    }

    if (nodeJson.contains("alignSelf")) {
        std::string as = nodeJson["alignSelf"].get<std::string>();
        if (as == "baseline") layoutNode->alignSelf = Align::Baseline;
        else if (as == "center") layoutNode->alignSelf = Align::Center;
        else if (as == "flex-start") layoutNode->alignSelf = Align::FlexStart;
        else if (as == "flex-end") layoutNode->alignSelf = Align::FlexEnd;
        else if (as == "stretch") layoutNode->alignSelf = Align::Stretch;
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

void VexUI::safeUpdate(const std::string& id, std::function<void(Widget*)> action) {
    if (initialized) {
        if (Widget* w = findById(m_root, id)) {
            action(w);
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
        if (!data) goto recurse;

        FontAtlas atlas;
        if (!stbtt_InitFont(&atlas.info, (unsigned char*)data->data.data(), 0)) goto recurse;

        int ascent, descent, lineGap;
        stbtt_GetFontVMetrics(&atlas.info, &ascent, &descent, &lineGap);

        float scale = stbtt_ScaleForPixelHeight(&atlas.info, pxSize);
        atlas.ascent = static_cast<float>(ascent) * scale;
        atlas.descent = static_cast<float>(descent) * scale;
        atlas.scale = scale;

        int texDim = 512;
        while (texDim < pxSize * 12 && texDim < 8192) texDim *= 2;

        std::vector<unsigned char> bitmap(texDim * texDim, 0);
        atlas.cdata.resize(96);
        stbtt_BakeFontBitmap((unsigned char*)data->data.data(), 0, pxSize, bitmap.data(), texDim, texDim, 32, 96, atlas.cdata.data());

        std::vector<unsigned char> rgba(texDim * texDim * 4);
        for (int i = 0; i < texDim * texDim; ++i) {
            unsigned char v = bitmap[i];
            v = (v > 127) ? 255 : 0;
            rgba[i*4 + 0] = 255; rgba[i*4 + 1] = 255; rgba[i*4 + 2] = 255; rgba[i*4 + 3] = v;
        }

        std::string texName = "ui_font_" + key;
        m_res->createTextureFromRaw(rgba, texDim, texDim, texName);

        atlas.texIdx = m_res->getTextureIndex(texName);
        atlas.width = texDim;
        atlas.height = texDim;
        atlas.bakedSize = pxSize;
        m_fontAtlases[key] = atlas;
    }
recurse:
    const auto& children = w->children;
    size_t count = children.size();
    for (size_t i = 0; i < count; ++i) {
        if (i + 1 < count) _mm_prefetch(reinterpret_cast<const char*>(children[i + 1]) + 64, _MM_HINT_T0);
        loadFonts(children[i]);
    }
}

void VexUI::loadImages(Widget* w) {
    if (!w) return;
    if (!w->image.empty()) m_res->loadTexture(GetAssetPath(w->image), GetAssetPath(w->image));
    const auto& children = w->children;
    size_t count = children.size();
    for (size_t i = 0; i < count; ++i) {
        if (i + 1 < count) _mm_prefetch(reinterpret_cast<const char*>(children[i + 1]) + 64, _MM_HINT_T0);
        loadImages(children[i]);
    }
}

Widget* VexUI::parseNode(const nlohmann::json& j) {
    Widget* w = new Widget();
    w->ui = this;
    w->layoutNode = new LayoutNode();
    w->layoutNode->context = w;

    w->nodeJson = j;
    w->applyLayout(this);

    if (j.contains("children") && j["children"].is_array()) {
        for (const auto& c : j["children"]) {
            Widget* child = parseNode(c);
            child->parent = w;

            if (child->type == WidgetType::Label || child->type == WidgetType::Button) {
                child->layoutNode->measureFunc = VexUI::measureTextNode;
            }

            if (!child->children.empty()) {
                if (child->layoutNode->alignSelf == Align::Auto) {
                    child->layoutNode->alignSelf = Align::Stretch;
                }
            }
            w->children.push_back(child);
            w->layoutNode->insertChild(child->layoutNode, w->layoutNode->children.size());
        }
    }
    return w;
}

void VexUI::load(const std::string& path) {
    if(initialized){
        std::string realPath = GetAssetPath(path);
        auto data = m_vfs->load_file(realPath);
        if (!data) return;

        nlohmann::json json;
        try { json = nlohmann::json::parse(data->data.begin(), data->data.end()); }
        catch (...) { return; }

        freeTree(m_root);
        m_root = nullptr;
        m_focusedWidget = nullptr;
        if (json.contains("root")) {
            m_root = parseNode(json["root"]);
            if (json["root"].contains("zindex")) zIndex = json["root"]["zindex"].get<int>();
        }
        if (m_root && json.contains("overlays") && json["overlays"].is_array()) {
            for (const auto& ov : json["overlays"]) {
                Widget* overlay = parseNode(ov);
                overlay->layoutNode->positionType = PositionType::Absolute;
                m_root->children.push_back(overlay);
                m_root->layoutNode->insertChild(overlay->layoutNode, m_root->layoutNode->children.size());
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
    if (m_root) calculateLayout(m_root->layoutNode, static_cast<float>(res.x), static_cast<float>(res.y), true, this);
}

Widget* VexUI::findWidgetAt(Widget* w, glm::vec2 pos, glm::vec2 parentOffset) {
    if (!w || !w->layoutNode) return nullptr;

    float absX = parentOffset.x + w->layoutNode->computedLeft;
    float absY = parentOffset.y + w->layoutNode->computedTop;
    float width  = w->layoutNode->computedWidth;
    float height = w->layoutNode->computedHeight;

    glm::vec2 pivot = { absX + width * 0.5f, absY + height * 0.5f };
    glm::vec2 checkPos = pos;
    if (w->rotation != 0.f) {
        float rads = glm::radians(-w->rotation);
        float c = cos(rads), s = sin(rads);
        glm::vec2 d = pos - pivot;
        checkPos.x = pivot.x + (d.x * c - d.y * s);
        checkPos.y = pivot.y + (d.x * s + d.y * c);
    }

    bool isInside = (checkPos.x >= absX && checkPos.x <= absX + width && checkPos.y >= absY && checkPos.y <= absY + height);

    for (auto it = w->children.rbegin(); it != w->children.rend(); ++it) {
        if (Widget* hit = findWidgetAt(*it, pos, {absX, absY})) return hit;
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
        glm::vec2 mouse(ev.button.x / sx, ev.button.y / sy);
        if (Widget* w = findWidgetAt(m_root, mouse, {0, 0}); w && w->type == WidgetType::Button && w->onClick) w->onClick();
    }
    else if (ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        SDL_GamepadButton button = (SDL_GamepadButton)ev.gbutton.button;
        if (button == SDL_GAMEPAD_BUTTON_SOUTH) {
            if (m_focusedWidget && m_focusedWidget->onClick) m_focusedWidget->onClick();
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
            if (!wasAbove && isAbove) navigateToWidget((value > 0.0f) ? 1.0f : -1.0f, 0.0f);
            m_gamepadAxisX = value;
        }
        else if (axis == SDL_GAMEPAD_AXIS_LEFTY) {
            bool wasAbove = std::abs(m_gamepadAxisY) > GAMEPAD_AXIS_THRESHOLD;
            bool isAbove = std::abs(value) > GAMEPAD_AXIS_THRESHOLD;
            if (!wasAbove && isAbove) navigateToWidget(0.0f, (value > 0.0f) ? 1.0f : -1.0f);
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
    if (!focusedIsValid) { setFocusedWidget(navigable.front()); return; }

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
    if (!w || !w->layoutNode) return {0.0f, 0.0f};

    float x = w->layoutNode->computedLeft;
    float y = w->layoutNode->computedTop;
    float width = w->layoutNode->computedWidth;
    float height = w->layoutNode->computedHeight;

    Widget* parent = w->parent;
    while (parent) {
        x += parent->layoutNode->computedLeft;
        y += parent->layoutNode->computedTop;
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

        if (isHorizontalNav) crossAlignment = std::max(0.0f, 1.0f - (std::abs(toCandidate.y) / 500.0f));
        else crossAlignment = std::max(0.0f, 1.0f - (std::abs(toCandidate.x) / 500.0f));

        if (dot > -0.2f) {
            float score = dot * 3.0f + crossAlignment * 0.5f - (distance / 800.0f);
            if (score > closestScore) { closestScore = score; closest = candidate; }
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

glm::vec2 VexUI::calculateTextSize(Widget* w, float maxWidth) {
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

void VexUI::measureTextNode(LayoutNode* node, float width, float height) {
    Widget* w = static_cast<Widget*>(node->context);
    if (!w || !w->ui) return;

    float maxW = (width > 0.0f && width != FLT_MAX && !std::isnan(width)) ? width : FLT_MAX;
    glm::vec2 measuredSize = w->ui->calculateTextSize(w, maxW);

    node->computedWidth = measuredSize.x;
    node->computedHeight = measuredSize.y;
}

void VexUI::batch(Widget* w, std::vector<float>& verts, glm::vec2 parentOffset) {
    if (!w || !w->layoutNode) return;

    float x = parentOffset.x + w->layoutNode->computedLeft;
    float y = parentOffset.y + w->layoutNode->computedTop;
    float width  = w->layoutNode->computedWidth;
    float height = w->layoutNode->computedHeight;

    glm::vec2 pivot = { x + width * 0.5f, y + height * 0.5f };
    float rads = glm::radians(w->rotation);
    float cosA = cos(rads);
    float sinA = sin(rads);

    auto pushQuad = [&](float x0, float y0, float u0, float v0, float x1, float y1, float u1, float v1, const glm::vec4& col, float texIdx) {
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

                float bPadL = w->layoutNode->paddingLeft.resolveOr(width, 0.f) + bw;
                float bPadR = w->layoutNode->paddingRight.resolveOr(width, 0.f) + bw;
                float bPadT = w->layoutNode->paddingTop.resolveOr(height, 0.f) + bw;
                float bPadB = w->layoutNode->paddingBottom.resolveOr(height, 0.f) + bw;

                float innerW = std::max(0.0f, width - bPadL - bPadR);
                float innerH = std::max(0.0f, height - bPadT - bPadB);

                std::vector<std::string> lines = wrapText(w->text, a, innerW);

                float lineHeight = a.ascent - a.descent;
                float totalTextHeight = lines.size() * lineHeight;
                float verticalOffset = (innerH - totalTextHeight) / 2.0f;

                float cy = y + bPadT + verticalOffset + a.ascent;

                for (const auto& line : lines) {
                    float lineWidth = 0.f;
                    for (char ch : line) {
                        if (ch < 32 || ch > 127) continue;
                        lineWidth += a.cdata[ch - 32].xadvance;
                    }

                    float startX = x + bPadL;
                    if (w->textAlign == TextAlign::Center) startX = x + bPadL + (innerW - lineWidth) / 2.f;
                    else if (w->textAlign == TextAlign::Right) startX = x + bPadL + (innerW - lineWidth);

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
                    cy += lineHeight;
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
        if (i + 1 < count) _mm_prefetch(reinterpret_cast<const char*>(children[i + 1]) + 64, _MM_HINT_T0);
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
