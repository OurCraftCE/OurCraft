#include "BedrockUIParser.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Types.h>

// nlohmann/json - header-only library
#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <filesystem>

using json = nlohmann::json;

// forwards click events to the parser's button callback
class ButtonEventListener : public Rml::EventListener {
public:
	ButtonEventListener(std::string buttonId, std::function<void(const std::string&)> callback)
		: m_buttonId(std::move(buttonId)), m_callback(std::move(callback)) {}

	void ProcessEvent(Rml::Event& event) override {
		if (event.GetId() == Rml::EventId::Click && m_callback) {
			m_callback(m_buttonId);
		}
	}

private:
	std::string m_buttonId;
	std::function<void(const std::string&)> m_callback;
};

static std::string Trim(const std::string& s) {
	size_t start = s.find_first_not_of(" \t\r\n");
	size_t end = s.find_last_not_of(" \t\r\n");
	return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

// split "name@namespace.ref" into name and extends
static void SplitControlKey(const std::string& key, std::string& outName, std::string& outExtends) {
	auto atPos = key.find('@');
	if (atPos != std::string::npos) {
		outName = Trim(key.substr(0, atPos));
		outExtends = Trim(key.substr(atPos + 1));
	} else {
		outName = Trim(key);
		outExtends.clear();
	}
}

bool BedrockUIParser::LoadFile(const std::string& filepath) {
	std::ifstream file(filepath);
	if (!file.is_open()) {
		Rml::Log::Message(Rml::Log::LT_WARNING, "[BedrockUIParser] Cannot open: %s", filepath.c_str());
		return false;
	}

	json root;
	try {
		root = json::parse(file, nullptr, true, true); // allow comments
	} catch (const json::parse_error& e) {
		Rml::Log::Message(Rml::Log::LT_WARNING, "[BedrockUIParser] JSON parse error in %s: %s",
		                  filepath.c_str(), e.what());
		return false;
	}

	// extract namespace from filename: "start_screen.json" -> "start_screen"
	std::filesystem::path p(filepath);
	std::string ns = p.stem().string();

	for (auto& [key, value] : root.items()) {
		if (key == "namespace") {
			ns = value.get<std::string>();
			continue;
		}
		if (!value.is_object())
			continue;

		std::string controlName, controlExtends;
		SplitControlKey(key, controlName, controlExtends);

		BedrockControl ctrl = ParseControlFromJson(&value, controlName);
		if (!controlExtends.empty())
			ctrl.extends = controlExtends;

		std::string fullName = ns + "." + controlName;
		m_controlDefs[fullName] = ctrl;
	}

	// register root_panel as the screen entry point
	std::string rootKey = ns + ".root_panel";
	if (m_controlDefs.count(rootKey)) {
		BedrockScreen screen;
		screen.name = ns;
		screen.namespace_name = ns;
		screen.root = m_controlDefs[rootKey];
		ResolveInheritance(screen.root);
		m_screens[ns] = std::move(screen);
	}

	return true;
}

bool BedrockUIParser::LoadUIDefinitions(const std::string& uiDir) {
	std::string defsPath = uiDir + "/_ui_defs.json";
	std::ifstream defsFile(defsPath);
	bool anyLoaded = false;

	if (defsFile.is_open()) {
		json defs;
		try {
			defs = json::parse(defsFile, nullptr, true, true);
		} catch (...) {
			defs = json();
		}

		if (defs.is_array()) {
			for (auto& entry : defs) {
				if (entry.is_string()) {
					std::string file = uiDir + "/" + entry.get<std::string>();
					if (LoadFile(file))
						anyLoaded = true;
				}
			}
			return anyLoaded;
		}
	}

	// fallback: load all .json files in directory
	try {
		for (auto& entry : std::filesystem::directory_iterator(uiDir)) {
			if (entry.path().extension() == ".json") {
				if (LoadFile(entry.path().string()))
					anyLoaded = true;
			}
		}
	} catch (...) {
		// directory doesn't exist or can't be iterated
	}

	return anyLoaded;
}

BedrockControl BedrockUIParser::ParseControlFromJson(const void* jsonPtr, const std::string& name) {
	const json& j = *static_cast<const json*>(jsonPtr);
	BedrockControl ctrl;
	ctrl.name = name;

	if (j.contains("type") && j["type"].is_string())
		ctrl.type = j["type"].get<std::string>();

	if (j.contains("text") && j["text"].is_string())
		ctrl.text = j["text"].get<std::string>();

	if (j.contains("texture") && j["texture"].is_string())
		ctrl.texture = j["texture"].get<std::string>();

	if (j.contains("size"))
		ParseSizeValue(&j["size"], ctrl);

	if (j.contains("offset"))
		ParseOffsetValue(&j["offset"], ctrl);

	if (j.contains("anchor_from") && j["anchor_from"].is_string())
		ctrl.anchor_from = j["anchor_from"].get<std::string>();
	if (j.contains("anchor_to") && j["anchor_to"].is_string())
		ctrl.anchor_to = j["anchor_to"].get<std::string>();

	if (j.contains("alpha") && j["alpha"].is_number())
		ctrl.alpha = j["alpha"].get<float>();

	if (j.contains("color"))
		ctrl.color = ParseColorArray(&j["color"]);

	if (j.contains("visible") && j["visible"].is_boolean())
		ctrl.visible = j["visible"].get<bool>();

	if (j.contains("layer") && j["layer"].is_number_integer())
		ctrl.zOrder = j["layer"].get<int>();

	// orientation is used by stack_panel
	if (j.contains("orientation") && j["orientation"].is_string())
		ctrl.orientation = j["orientation"].get<std::string>();

	if (j.contains("spacing") && j["spacing"].is_number())
		ctrl.spacing = j["spacing"].get<float>();

	if (j.contains("button_id") && j["button_id"].is_string())
		ctrl.buttonId = j["button_id"].get<std::string>();

	if (j.contains("bindings"))
		ParseBindings(&j["bindings"], ctrl);

	if (j.contains("button_mappings") && j["button_mappings"].is_array()) {
		for (auto& mapping : j["button_mappings"]) {
			if (mapping.is_object()) {
				std::string desc;
				if (mapping.contains("from_button_id") && mapping["from_button_id"].is_string())
					desc = mapping["from_button_id"].get<std::string>();
				if (!desc.empty())
					ctrl.buttonMappings.push_back(desc);
			}
		}
	}

	if (j.contains("controls"))
		ParseChildren(&j["controls"], ctrl);

	return ctrl;
}

void BedrockUIParser::ParseSizeValue(const void* jsonPtr, BedrockControl& ctrl) {
	const json& j = *static_cast<const json*>(jsonPtr);
	if (!j.is_array() || j.size() < 2)
		return;

	for (int i = 0; i < 2; i++) {
		if (j[i].is_string()) {
			std::string val = j[i].get<std::string>();
			if (val.find('%') != std::string::npos) {
				ctrl.sizeIsPercent[i] = true;
				ctrl.size[i] = std::stof(val);
			} else if (val == "fill" || val == "default") {
				ctrl.sizeIsPercent[i] = true;
				ctrl.size[i] = 100.0f;
			} else {
				ctrl.size[i] = std::stof(val);
			}
		} else if (j[i].is_number()) {
			ctrl.size[i] = j[i].get<float>();
		}
	}
}

void BedrockUIParser::ParseOffsetValue(const void* jsonPtr, BedrockControl& ctrl) {
	const json& j = *static_cast<const json*>(jsonPtr);
	if (!j.is_array() || j.size() < 2)
		return;

	for (int i = 0; i < 2; i++) {
		if (j[i].is_number())
			ctrl.offset[i] = j[i].get<float>();
		else if (j[i].is_string())
			ctrl.offset[i] = std::stof(j[i].get<std::string>());
	}
}

std::string BedrockUIParser::ParseColorArray(const void* jsonPtr) {
	const json& j = *static_cast<const json*>(jsonPtr);

	// Bedrock color: [r,g,b] with 0-1 float or 0-255 int, or a string
	if (j.is_string())
		return j.get<std::string>();

	if (j.is_array() && j.size() >= 3) {
		float r = j[0].get<float>();
		float g = j[1].get<float>();
		float b = j[2].get<float>();

		// if all values <= 1.0, assume 0-1 range
		if (r <= 1.0f && g <= 1.0f && b <= 1.0f) {
			r *= 255.0f;
			g *= 255.0f;
			b *= 255.0f;
		}

		char buf[32];
		snprintf(buf, sizeof(buf), "rgb(%d,%d,%d)",
		         (int)r, (int)g, (int)b);
		return buf;
	}

	return "";
}

void BedrockUIParser::ParseBindings(const void* jsonPtr, BedrockControl& ctrl) {
	const json& j = *static_cast<const json*>(jsonPtr);
	if (!j.is_array())
		return;

	for (auto& b : j) {
		if (!b.is_object())
			continue;

		BedrockControl::Binding binding;
		if (b.contains("binding_name") && b["binding_name"].is_string())
			binding.bindingName = b["binding_name"].get<std::string>();
		if (b.contains("binding_name_override") && b["binding_name_override"].is_string())
			binding.bindingNameOverride = b["binding_name_override"].get<std::string>();
		if (b.contains("binding_type") && b["binding_type"].is_string())
			binding.bindingType = b["binding_type"].get<std::string>();
		if (b.contains("source_property_name") && b["source_property_name"].is_string())
			binding.sourcePropertyName = b["source_property_name"].get<std::string>();
		if (b.contains("source_control_name") && b["source_control_name"].is_string())
			binding.sourceControlName = b["source_control_name"].get<std::string>();

		ctrl.bindings.push_back(std::move(binding));
	}
}

void BedrockUIParser::ParseChildren(const void* jsonPtr, BedrockControl& ctrl) {
	const json& j = *static_cast<const json*>(jsonPtr);
	if (!j.is_array())
		return;

	for (auto& child : j) {
		if (!child.is_object())
			continue;

		// each child is an object with a single key: {"name@ref": { ... }}
		for (auto& [key, value] : child.items()) {
			std::string childName, childExtends;
			SplitControlKey(key, childName, childExtends);

			BedrockControl childCtrl;
			if (value.is_object()) {
				childCtrl = ParseControlFromJson(&value, childName);
			} else {
				childCtrl.name = childName;
			}

			if (!childExtends.empty())
				childCtrl.extends = childExtends;

			ctrl.controls.push_back(std::move(childCtrl));
		}
	}
}

void BedrockUIParser::ResolveInheritance(BedrockControl& control) {
	if (!control.extends.empty()) {
		auto it = m_controlDefs.find(control.extends);
		if (it != m_controlDefs.end()) {
			const BedrockControl& base = it->second;

			// inherit empty fields from base
			if (control.type.empty())
				control.type = base.type;
			if (control.text.empty())
				control.text = base.text;
			if (control.texture.empty())
				control.texture = base.texture;
			if (control.size[0] == 0 && control.size[1] == 0) {
				control.size[0] = base.size[0];
				control.size[1] = base.size[1];
				control.sizeIsPercent[0] = base.sizeIsPercent[0];
				control.sizeIsPercent[1] = base.sizeIsPercent[1];
			}
			if (control.offset[0] == 0 && control.offset[1] == 0) {
				control.offset[0] = base.offset[0];
				control.offset[1] = base.offset[1];
			}
			if (control.anchor_from.empty())
				control.anchor_from = base.anchor_from;
			if (control.anchor_to.empty())
				control.anchor_to = base.anchor_to;
			if (control.alpha == 1.0f)
				control.alpha = base.alpha;
			if (control.color.empty())
				control.color = base.color;
			if (control.orientation.empty())
				control.orientation = base.orientation;
			if (control.buttonId.empty())
				control.buttonId = base.buttonId;
			if (control.bindings.empty())
				control.bindings = base.bindings;
			if (control.buttonMappings.empty())
				control.buttonMappings = base.buttonMappings;

			// merge children: base first, then overrides
			if (control.controls.empty() && !base.controls.empty())
				control.controls = base.controls;
		}
	}

	for (auto& child : control.controls)
		ResolveInheritance(child);
}

const BedrockScreen* BedrockUIParser::GetScreen(const std::string& name) const {
	auto it = m_screens.find(name);
	return (it != m_screens.end()) ? &it->second : nullptr;
}

Rml::ElementDocument* BedrockUIParser::CreateDocument(Rml::Context* context, const std::string& screenName) {
	if (!context)
		return nullptr;

	const BedrockScreen* screen = GetScreen(screenName);
	if (!screen) {
		Rml::Log::Message(Rml::Log::LT_WARNING, "[BedrockUIParser] Screen not found: %s", screenName.c_str());
		return nullptr;
	}

	// build an RML document string with embedded RCSS
	std::string rml;
	rml += "<rml>\n<head>\n<style>\n";

	rml += "body { display: block; width: 100%; height: 100%; }\n";
	rml += ".bedrock-panel { display: block; position: relative; }\n";
	rml += ".bedrock-stack-v { display: flex; flex-direction: column; }\n";
	rml += ".bedrock-stack-h { display: flex; flex-direction: row; }\n";
	rml += ".bedrock-label { display: inline; }\n";
	rml += ".bedrock-button { display: block; cursor: pointer; }\n";
	rml += ".bedrock-button:hover { opacity: 0.8; }\n";
	rml += ".bedrock-image { display: block; }\n";
	rml += ".bedrock-scroll { display: block; overflow: auto; }\n";
	rml += ".bedrock-grid { display: block; }\n";
	rml += ".bedrock-input { display: block; }\n";
	rml += ".bedrock-hidden { display: none; }\n";

	rml += "</style>\n</head>\n<body id=\"" + screenName + "\">\n";

	rml += "</body>\n</rml>\n";

	Rml::ElementDocument* doc = context->LoadDocumentFromMemory(rml, screenName);
	if (!doc) {
		Rml::Log::Message(Rml::Log::LT_WARNING, "[BedrockUIParser] Failed to create document for: %s", screenName.c_str());
		return nullptr;
	}

	BuildRmlElement(doc, screen->root, doc);

	doc->Show();
	return doc;
}

void BedrockUIParser::BuildRmlElement(Rml::Element* parent, const BedrockControl& control, Rml::ElementDocument* doc) {
	if (!parent || !doc)
		return;

	// map Bedrock control type to RmlUi tag
	std::string tag;
	std::string cssClass;

	if (control.type == "label") {
		tag = "p";
		cssClass = "bedrock-label";
	} else if (control.type == "button") {
		tag = "button";
		cssClass = "bedrock-button";
	} else if (control.type == "image") {
		tag = "div";
		cssClass = "bedrock-image";
	} else if (control.type == "stack_panel") {
		tag = "div";
		cssClass = (control.orientation == "horizontal") ? "bedrock-stack-h" : "bedrock-stack-v";
	} else if (control.type == "grid") {
		tag = "div";
		cssClass = "bedrock-grid";
	} else if (control.type == "scroll_view") {
		tag = "div";
		cssClass = "bedrock-scroll";
	} else if (control.type == "input_panel") {
		tag = "input";
		cssClass = "bedrock-input";
	} else {
		// panel or unknown
		tag = "div";
		cssClass = "bedrock-panel";
	}

	Rml::ElementPtr elem = doc->CreateElement(tag);
	if (!elem)
		return;

	Rml::Element* rawElem = elem.get();

	if (!control.name.empty())
		rawElem->SetId(control.name);

	if (!cssClass.empty())
		rawElem->SetAttribute("class", cssClass);

	if (!control.visible)
		rawElem->SetClass("bedrock-hidden", true);

	std::string style = GenerateRCSS(control);
	if (!style.empty())
		rawElem->SetAttribute("style", style);

	if (control.type == "label" && !control.text.empty()) {
		std::string resolvedText = control.text;

		// resolve binding reference if text starts with '#'
		if (!control.text.empty() && control.text[0] == '#' && m_bindingCallback) {
			std::string bound = m_bindingCallback(control.text);
			if (!bound.empty())
				resolvedText = bound;
		}

		rawElem->SetInnerRML(resolvedText);
	}

	for (auto& binding : control.bindings) {
		if (!binding.bindingNameOverride.empty() && m_bindingCallback) {
			std::string value = m_bindingCallback(binding.bindingName);
			if (!value.empty()) {
				if (binding.bindingNameOverride.find("_text") != std::string::npos ||
				    binding.bindingNameOverride == "#text") {
					rawElem->SetInnerRML(value);
				}
			}
		}
		// store binding name for later dynamic updates
		if (!binding.bindingName.empty())
			rawElem->SetAttribute("data-binding", binding.bindingName);
	}

	if (control.type == "image" && !control.texture.empty()) {
		// use decorator for background image in RmlUi
		std::string existingStyle = rawElem->GetAttribute<Rml::String>("style", "");
		if (!existingStyle.empty() && existingStyle.back() != ';')
			existingStyle += "; ";
		existingStyle += "decorator: image(" + control.texture + ")";
		rawElem->SetAttribute("style", existingStyle);
	}

	if (control.type == "button" && !control.buttonId.empty() && m_buttonCallback) {
		// leaks intentionally — listener must outlive element; use a pool in production
		auto* listener = new ButtonEventListener(control.buttonId, m_buttonCallback);
		rawElem->AddEventListener("click", listener);

		if (!control.text.empty())
			rawElem->SetInnerRML(control.text);
	}

	for (auto& child : control.controls) {
		BuildRmlElement(rawElem, child, doc);
	}

	parent->AppendChild(std::move(elem));
}

std::string BedrockUIParser::GenerateRCSS(const BedrockControl& control) {
	std::ostringstream css;

	if (control.size[0] > 0) {
		if (control.sizeIsPercent[0])
			css << "width: " << control.size[0] << "%; ";
		else
			css << "width: " << control.size[0] << "px; ";
	}
	if (control.size[1] > 0) {
		if (control.sizeIsPercent[1])
			css << "height: " << control.size[1] << "%; ";
		else
			css << "height: " << control.size[1] << "px; ";
	}

	bool hasAnchoring = !control.anchor_from.empty() || !control.anchor_to.empty();
	if (hasAnchoring || control.offset[0] != 0 || control.offset[1] != 0) {
		std::string anchorCSS = AnchorToCSS(control.anchor_from, control.anchor_to,
		                                     control.offset[0], control.offset[1]);
		if (!anchorCSS.empty())
			css << anchorCSS;
	}

	if (control.alpha < 1.0f) {
		css << "opacity: " << control.alpha << "; ";
	}

	if (!control.color.empty()) {
		css << "color: " << control.color << "; ";
	}

	if (control.type == "stack_panel" && control.spacing > 0) {
		css << "gap: " << control.spacing << "px; ";
	}

	if (control.zOrder != 0) {
		css << "z-index: " << control.zOrder << "; ";
	}

	return css.str();
}

std::string BedrockUIParser::AnchorToCSS(const std::string& anchorFrom, const std::string& anchorTo,
                                          float offsetX, float offsetY) {
	std::ostringstream css;

	if (anchorFrom.empty() && anchorTo.empty()) {
		// no anchoring — apply offset as margin
		if (offsetX != 0)
			css << "margin-left: " << offsetX << "px; ";
		if (offsetY != 0)
			css << "margin-top: " << offsetY << "px; ";
		return css.str();
	}

	css << "position: absolute; ";

	// anchorTo: where in parent the element is placed
	// anchorFrom: which point of the element aligns to that anchor
	std::string to = anchorTo.empty() ? anchorFrom : anchorTo;
	std::string from = anchorFrom.empty() ? anchorTo : anchorFrom;

	if (to == "top_left") {
		css << "left: " << offsetX << "px; top: " << offsetY << "px; ";
	} else if (to == "top_middle") {
		css << "left: 50%; top: " << offsetY << "px; ";
	} else if (to == "top_right") {
		css << "right: " << -offsetX << "px; top: " << offsetY << "px; ";
	} else if (to == "left_middle") {
		css << "left: " << offsetX << "px; top: 50%; ";
	} else if (to == "center") {
		css << "left: 50%; top: 50%; ";
	} else if (to == "right_middle") {
		css << "right: " << -offsetX << "px; top: 50%; ";
	} else if (to == "bottom_left") {
		css << "left: " << offsetX << "px; bottom: " << -offsetY << "px; ";
	} else if (to == "bottom_middle") {
		css << "left: 50%; bottom: " << -offsetY << "px; ";
	} else if (to == "bottom_right") {
		css << "right: " << -offsetX << "px; bottom: " << -offsetY << "px; ";
	} else {
		css << "left: " << offsetX << "px; top: " << offsetY << "px; ";
	}

	// transform adjusts for anchor_from (origin of the element)
	float tx = 0, ty = 0;
	if (from == "top_left") {
		tx = 0; ty = 0;
	} else if (from == "top_middle") {
		tx = -50; ty = 0;
	} else if (from == "top_right") {
		tx = -100; ty = 0;
	} else if (from == "left_middle") {
		tx = 0; ty = -50;
	} else if (from == "center") {
		tx = -50; ty = -50;
	} else if (from == "right_middle") {
		tx = -100; ty = -50;
	} else if (from == "bottom_left") {
		tx = 0; ty = -100;
	} else if (from == "bottom_middle") {
		tx = -50; ty = -100;
	} else if (from == "bottom_right") {
		tx = -100; ty = -100;
	}

	if (tx != 0 || ty != 0) {
		css << "transform: translate(" << tx << "%, " << ty << "%); ";
	}

	return css.str();
}
