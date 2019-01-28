#include "Hierarchy.hpp"

#include <Devices/Keyboard.hpp>
#include <Maths/Visual/DriverConstant.hpp>

namespace test
{
	Hierarchy::Hierarchy(UiObject *parent) :
		UiPanel(parent, UiBound(Vector2(0.0f, 0.0f), UiReference::TopLeft, UiAspect::Dimensions, Vector2(0.4f, 1.0f)), ScrollBar::Vertical),
		m_section1(std::make_unique<UiSection>(GetContent(), "Section Left", UiBound(Vector2(0.08f, 0.05f), UiReference::TopLeft))),
		m_button1(std::make_unique<UiInputButton>(m_section1->GetContent(), "Button #1", UiBound(Vector2(0.0f, 0.0f), UiReference::TopLeft))),
		m_sliderR(std::make_unique<UiInputSlider>(m_section1->GetContent(), "R", 255.0f, 0.0f, 255.0f, 0, UiBound(Vector2(0.0f, 0.07f), UiReference::TopLeft))),
		m_sliderG(std::make_unique<UiInputSlider>(m_section1->GetContent(), "G", 255.0f, 0.0f, 255.0f, 0, UiBound(Vector2(0.0f, 0.14f), UiReference::TopLeft))),
		m_sliderB(std::make_unique<UiInputSlider>(m_section1->GetContent(), "B", 0.0f, 0.0f, 255.0f, 0, UiBound(Vector2(0.0f, 0.21f), UiReference::TopLeft))),
		m_textHex(std::make_unique<UiInputText>(m_section1->GetContent(), "Hex", Colour::Yellow.GetHex(), 14, UiBound(Vector2(0.0f, 0.28f), UiReference::TopLeft))),
		m_rgbColour(std::make_unique<Gui>(m_section1->GetContent(), UiBound(Vector2(0.0f, 0.35f), UiReference::TopLeft, UiAspect::Position | UiAspect::Dimensions, Vector2(0.3f, 0.045f)), Texture::Create("Guis/Button_Filled.png"), Colour::Yellow)),
		m_colourWheel(std::make_unique<UiColourWheel>(m_section1->GetContent(), Colour::Yellow, UiBound(Vector2(0.0f, 0.42f), UiReference::TopLeft)))
	{
		m_button1->GetOnClick() += [](UiObject *object, MouseButton button) {
			if (button == MouseButton::Left)
			{
				Log::Out("Button #1 pressed!\n");
			}
		};

		auto onSlide = [this](UiInputSlider *object, float value) {
			auto colour = Colour(m_sliderR->GetValue(), m_sliderG->GetValue(), m_sliderB->GetValue()) / 255.0f;
			dynamic_cast<DriverConstant<Colour> *>(m_rgbColour->GetColourDriver())->SetConstant(colour);
			m_colourWheel->SetColour(colour);
			m_textHex->SetValue(colour.GetHex());
		};
		m_sliderR->GetOnSlide() += onSlide;
		m_sliderG->GetOnSlide() += onSlide;
		m_sliderB->GetOnSlide() += onSlide;

		/*m_colourWheel->GetOnPressed() += [this](UiColourWheel *object, Colour value) {
			m_sliderR->SetValue(255.0f * value.m_r);
			m_sliderG->SetValue(255.0f * value.m_g);
			m_sliderB->SetValue(255.0f * value.m_b);
			dynamic_cast<DriverConstant<Colour> *>(m_rgbColour->GetColourDriver())->SetConstant(value);
		};*/
	}

	void Hierarchy::UpdateObject()
	{
		UiPanel::UpdateObject();
	}
}
