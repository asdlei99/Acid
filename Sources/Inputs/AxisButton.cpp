#include "AxisButton.hpp"

namespace acid
{
AxisButton::AxisButton(IButton *negative, IButton *positive) :
	m_negative(negative),
	m_positive(positive)
{
	m_negative->GetOnButton() += [this](InputAction action, BitMask<InputMod> mods)
	{
		m_onAxis(GetAmount());
	};
	m_positive->GetOnButton() += [this](InputAction action, BitMask<InputMod> mods)
	{
		m_onAxis(GetAmount());
	};
}

float AxisButton::GetAmount() const
{
	auto result = 0.0f;

	if (m_positive->IsDown())
	{
		result += 1.0f;
	}

	if (m_negative->IsDown())
	{
		result -= 1.0f;
	}

	return result;
}
}
