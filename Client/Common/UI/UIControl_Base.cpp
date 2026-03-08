#include "stdafx.h"
#include "UI.h"
#include "UIControl.h"
#include "..\..\..\World\Math\StringHelpers.h"
#include "..\..\..\World\Math\JavaMath.h"

UIControl_Base::UIControl_Base()
{
	m_bLabelChanged = false;
	m_label = L"";
	m_id = 0;
}

bool UIControl_Base::setupControl(UIScene *scene, IggyValuePath *parent, const string &controlName)
{
	bool success = UIControl::setupControl(scene, parent, controlName);

	// Fast-names are no-ops now
	m_setLabelFunc         = registerFastName(L"SetLabel");
	m_initFunc             = registerFastName(L"Init");
	m_funcGetLabel         = registerFastName(L"GetLabel");
	m_funcCheckLabelWidths = registerFastName(L"CheckLabelWidths");

	return success;
}

void UIControl_Base::tick()
{
	UIControl::tick();

	if (m_bLabelChanged)
	{
		m_bLabelChanged = false;
		// Apply label to RmlUi element's inner text
		if (m_element)
		{
			string utf8(m_label.begin(), m_label.end());
			m_element->SetInnerRML(utf8);
		}
	}
}

void UIControl_Base::setLabel(const wstring &label, bool instant, bool force)
{
	if (force || ((!m_label.empty() || !label.empty()) && m_label.compare(label) != 0))
		m_bLabelChanged = true;
	m_label = label;

	if (m_bLabelChanged && instant)
	{
		m_bLabelChanged = false;
		if (m_element)
		{
			string utf8(m_label.begin(), m_label.end());
			m_element->SetInnerRML(utf8);
		}
	}
}

void UIControl_Base::setLabel(const string &label)
{
	setLabel(convStringToWstring(label));
}

const wchar_t *UIControl_Base::getLabel()
{
	if (m_element)
	{
		string inner = m_element->GetInnerRML();
		m_label = wstring(inner.begin(), inner.end());
	}
	return m_label.c_str();
}

void UIControl_Base::setAllPossibleLabels(int labelCount, wchar_t labels[][256])
{
	// No-op in RmlUi — label sizing is handled by CSS
}

bool UIControl_Base::hasFocus()
{
	return m_parentScene->controlHasFocus(this);
}
