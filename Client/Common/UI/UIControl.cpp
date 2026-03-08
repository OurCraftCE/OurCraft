#include "stdafx.h"
#include "UI.h"
#include "UIControl.h"
#include "..\..\..\World\Math\StringHelpers.h"
#include "..\..\..\World\Math\JavaMath.h"

UIControl::UIControl()
{
	m_parentScene = nullptr;
	m_element = nullptr;
	m_lastOpacity = 1.0f;
	m_controlName = "";
	m_isVisible = true;
	m_bHidden = false;
	m_eControlType = eNoControl;
	m_x = m_y = m_width = m_height = 0;
}

bool UIControl::setupControl(UIScene *scene, IggyValuePath *parent, const string &controlName)
{
	m_parentScene = scene;
	m_controlName = controlName;

	// Iggy stub call (no-op)
	IggyValuePathMakeNameRef(&m_iggyPath, parent, controlName.c_str());

	// RmlUi — find element by ID in the document
	Rml::ElementDocument *doc = scene->getDocument();
	if (doc)
	{
		m_element = doc->GetElementById(controlName);
		if (m_element)
		{
			m_x      = (S32)m_element->GetAbsoluteLeft();
			m_y      = (S32)m_element->GetAbsoluteTop();
			m_width  = (S32)m_element->GetClientWidth();
			m_height = (S32)m_element->GetClientHeight();
		}
	}

	m_nameXPos   = registerFastName(L"x");
	m_nameYPos   = registerFastName(L"y");
	m_nameWidth  = registerFastName(L"width");
	m_nameHeight = registerFastName(L"height");
	m_funcSetAlpha = registerFastName(L"SetControlAlpha");
	m_nameVisible  = registerFastName(L"visible");

	return m_element != nullptr;
}

void UIControl::ReInit()
{
	// Re-find element after document reload
	if (m_parentScene && m_parentScene->getDocument())
		m_element = m_parentScene->getDocument()->GetElementById(m_controlName);

	if (m_element)
	{
		if (m_lastOpacity != 1.0f)
			m_element->SetProperty("opacity", Rml::ToString(m_lastOpacity));
		m_element->SetProperty("visibility", m_isVisible ? "visible" : "hidden");
	}
}

S32 UIControl::getXPos()    { return m_element ? (S32)m_element->GetAbsoluteLeft()   : m_x; }
S32 UIControl::getYPos()    { return m_element ? (S32)m_element->GetAbsoluteTop()    : m_y; }
S32 UIControl::getWidth()   { return m_element ? (S32)m_element->GetClientWidth()    : m_width; }
S32 UIControl::getHeight()  { return m_element ? (S32)m_element->GetClientHeight()   : m_height; }

void UIControl::setOpacity(float percent)
{
	if (percent == m_lastOpacity) return;
	m_lastOpacity = percent;
	if (m_element)
		m_element->SetProperty("opacity", Rml::ToString(percent));
}

void UIControl::setVisible(bool visible)
{
	if (!m_parentScene) return;
	m_isVisible = visible;
	if (m_element)
		m_element->SetProperty("visibility", visible ? "visible" : "hidden");
}

bool UIControl::getVisible()
{
	if (m_element)
	{
		const Rml::Property *prop = m_element->GetProperty("visibility");
		m_isVisible = prop && prop->Get<Rml::String>() != "hidden";
	}
	return m_isVisible;
}

IggyName UIControl::registerFastName(const wstring &name)
{
	return m_parentScene ? m_parentScene->registerFastName(name) : IggyName{};
}
