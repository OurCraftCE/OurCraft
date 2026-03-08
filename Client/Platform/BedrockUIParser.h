#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace Rml { class ElementDocument; class Element; class Context; }

struct BedrockControl {
	std::string type;           // "panel", "label", "button", "image", "stack_panel", "grid", "scroll_view", "input_panel", "custom"
	std::string name;
	std::string extends;        // base control to inherit from (e.g., "common.button")

	std::string text;
	std::string texture;
	float size[2] = {0, 0};    // width, height (absolute or percentage)
	bool sizeIsPercent[2] = {false, false};
	float offset[2] = {0, 0};
	std::string anchor_from;    // "top_left", "center", etc.
	std::string anchor_to;
	float alpha = 1.0f;
	std::string color;          // CSS-style color
	bool visible = true;
	std::string layer;
	int zOrder = 0;

	std::string orientation;    // "vertical" or "horizontal" for stack_panel
	float spacing = 0;

	struct Binding {
		std::string bindingName;         // "#text"
		std::string bindingNameOverride; // "#label_text"
		std::string bindingType;         // "global", "collection"
		std::string sourcePropertyName;
		std::string sourceControlName;
	};
	std::vector<Binding> bindings;

	std::vector<BedrockControl> controls;

	std::string buttonId;
	std::vector<std::string> buttonMappings;
};

struct BedrockScreen {
	std::string name;
	std::string namespace_name;
	BedrockControl root;
};

class BedrockUIParser {
public:
	bool LoadFile(const std::string& filepath);

	// reads _ui_defs.json first, then all referenced files
	bool LoadUIDefinitions(const std::string& uiDir);

	Rml::ElementDocument* CreateDocument(Rml::Context* context, const std::string& screenName);

	using BindingCallback = std::function<std::string(const std::string& bindingName)>;
	void SetBindingCallback(BindingCallback callback) { m_bindingCallback = std::move(callback); }

	using ButtonCallback = std::function<void(const std::string& buttonId)>;
	void SetButtonCallback(ButtonCallback callback) { m_buttonCallback = std::move(callback); }

	const BedrockScreen* GetScreen(const std::string& name) const;

private:
	// void* avoids pulling json.hpp into headers
	BedrockControl ParseControlFromJson(const void* jsonPtr, const std::string& name);
	void ParseSizeValue(const void* jsonPtr, BedrockControl& ctrl);
	void ParseOffsetValue(const void* jsonPtr, BedrockControl& ctrl);
	std::string ParseColorArray(const void* jsonPtr);
	void ParseBindings(const void* jsonPtr, BedrockControl& ctrl);
	void ParseChildren(const void* jsonPtr, BedrockControl& ctrl);
	void ResolveInheritance(BedrockControl& control);
	void BuildRmlElement(Rml::Element* parent, const BedrockControl& control, Rml::ElementDocument* doc);
	std::string GenerateRCSS(const BedrockControl& control);
	std::string AnchorToCSS(const std::string& anchorFrom, const std::string& anchorTo,
	                        float offsetX, float offsetY);

	std::unordered_map<std::string, BedrockScreen> m_screens;
	std::unordered_map<std::string, BedrockControl> m_controlDefs;

	BindingCallback m_bindingCallback;
	ButtonCallback m_buttonCallback;
};
