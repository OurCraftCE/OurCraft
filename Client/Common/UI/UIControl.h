#pragma once
#include <RmlUi/Core/Element.h>

class UIControl
{
public:
	enum eUIControlType
	{
		eNoControl,
		eButton,
		eButtonList,
		eCheckBox,
		eCursor,
		eDLCList,
		eDynamicLabel,
		eEnchantmentBook,
		eEnchantmentButton,
		eHTMLLabel,
		eLabel,
		eLeaderboardList,
		eMinecraftPlayer,
		ePlayerList,
		ePlayerSkinPreview,
		eProgress,
		eSaveList,
		eSlider,
		eSlotList,
		eTextInput,
		eTexturePackList,
		eBitmapIcon,
		eTouchControl,
	};

protected:
	eUIControlType m_eControlType;
	int m_id;
	bool m_bHidden;

	// RmlUi element — null until scene's .rml is created
	Rml::Element *m_element;

	UIScene *m_parentScene;
	string m_controlName;

	// Iggy compat — kept for ABI, unused at runtime
	IggyValuePath m_iggyPath;
	IggyName m_nameXPos, m_nameYPos, m_nameWidth, m_nameHeight;
	IggyName m_funcSetAlpha, m_nameVisible;

	S32 m_x, m_y, m_width, m_height;
	float m_lastOpacity;
	bool m_isVisible;

public:
	UIControl();

	void setControlType(eUIControlType eType) { m_eControlType = eType; }
	eUIControlType getControlType() { return m_eControlType; }
	void setId(int iID) { m_id = iID; }
	int getId() { return m_id; }
	UIScene *getParentScene() { return m_parentScene; }

	// Setup — Iggy parent path kept for compat with existing subclass overrides
	virtual bool setupControl(UIScene *scene, IggyValuePath *parent, const string &controlName);

	// RmlUi element access
	Rml::Element *getElement() { return m_element; }

	// Iggy compat — returns address of stub path
	IggyValuePath *getIggyValuePath() { return &m_iggyPath; }

	string getControlName() { return m_controlName; }

	virtual void tick() {}
	virtual void ReInit();
	virtual void setFocus(bool focus) {}

	S32 getXPos();
	S32 getYPos();
	S32 getWidth();
	S32 getHeight();

	void setOpacity(float percent);
	void setVisible(bool visible);
	bool getVisible();
	bool isVisible() { return m_isVisible; }
	virtual bool hasFocus() { return false; }

protected:
	IggyName registerFastName(const wstring &name);
};
