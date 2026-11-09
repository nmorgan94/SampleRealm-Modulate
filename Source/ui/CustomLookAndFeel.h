#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "ModalOverlay.h"

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:

    CustomLookAndFeel();

    static constexpr float regularWeight = 400.0f;
    static constexpr float boldWeight = 700.0f;

    // From the one variable font
    [[nodiscard]] static juce::FontOptions orbitron(float weight);
    
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                         bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);

        if (button.getProperties().getWithDefault("pillToggle", false))
        {
            auto neonGreen = juce::Colour(0xff00ff41);
            auto greyBorder = juce::Colour(0xff3a3a3a);

            // Draw rounded rectangle border
            g.setColour(button.getToggleState() ? neonGreen : greyBorder);
            g.drawRoundedRectangle(bounds, 3.0f, 1.5f);

            // Draw text
            g.setColour(button.getToggleState() ? neonGreen : juce::Colour(0xff666666));
            g.setFont(orbitron(regularWeight).withHeight(10.0f));
            g.drawText(button.getButtonText(), bounds, juce::Justification::centred);

            return;
        }
        
        auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
        auto circleBounds = bounds.withSizeKeepingCentre(size, size);
        
        auto neonGreen = juce::Colour(0xff00ff41);
        auto darkGreen = juce::Colour(0xff00aa2e);
        auto greyBorder = juce::Colour(0xff2a2a2a);
        
        // Draw outer circle (border)
        g.setColour(button.getToggleState() ? neonGreen : greyBorder);
        g.drawEllipse(circleBounds, 2.0f);
        
        // Draw inner fill if enabled
        if (button.getToggleState())
        {
            // Dark green glow fill
            auto innerBounds = circleBounds.reduced(3.0f);
            g.setColour(darkGreen.withAlpha(0.3f));
            g.fillEllipse(innerBounds);
            
            // Draw center dot with glow effect
            auto dotBounds = circleBounds.reduced(size * 0.35f);
            
            // Outer glow
            g.setColour(neonGreen.withAlpha(0.4f));
            g.fillEllipse(dotBounds.expanded(2.0f));
            
            // Inner bright dot
            g.setColour(neonGreen);
            g.fillEllipse(dotBounds);
        }
    }
    
    void drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                     int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                     juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
        
        // Background
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Border
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        
        // Arrow on right side — fixed-size triangle, centred in the box height
        auto arrowZone = bounds.removeFromRight(20.0f);
        auto arrowBounds = juce::Rectangle<float>(10.0f, 6.0f)
                               .withCentre(arrowZone.getCentre());
        juce::Path arrow;
        arrow.addTriangle(arrowBounds.getX(), arrowBounds.getY(),
                         arrowBounds.getRight(), arrowBounds.getY(),
                         arrowBounds.getCentreX(), arrowBounds.getBottom());
        
        g.setColour(box.isEnabled() ? juce::Colour(0xff00ff41) : juce::Colour(0xff666666));
        g.fillPath(arrow);
    }
    
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
    {
        g.fillAll(juce::Colour(0xff2a2a2a));
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRect(0, 0, width, height, 1);
    }
    
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                          bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                          bool hasSubMenu, const juce::String& text,
                          const juce::String& /*shortcutKeyText*/,
                          const juce::Drawable* /*icon*/, const juce::Colour* /*textColour*/) override
    {
        if (isSeparator)
        {
            g.setColour(juce::Colour(0xff3a3a3a));
            g.fillRect(area.withSizeKeepingCentre(area.getWidth() - 16, 1));
            return;
        }

        // Ticked items, such as the current preset, are green instead of ticked
        auto colour = isTicked ? juce::Colour(0xff00ff41) : juce::Colours::white;

        if (! isActive)
        {
            colour = juce::Colour(0xff666666);
        }
        else if (isHighlighted)
        {
            g.setColour(juce::Colour(0xff00ff41).withAlpha(0.2f));
            g.fillRect(area);
            colour = juce::Colour(0xff00ff41);
        }

        auto r = area.reduced(10, 0);

        if (hasSubMenu)
        {
            const auto arrowArea = r.removeFromRight(8).toFloat().withSizeKeepingCentre(6.0f, 8.0f);
            juce::Path arrow;
            arrow.addTriangle(arrowArea.getTopLeft(), arrowArea.getBottomLeft(),
                              { arrowArea.getRight(), arrowArea.getCentreY() });
            g.setColour(juce::Colour(0xff00ff41));
            g.fillPath(arrow);
        }

        g.setColour(colour);
        g.setFont(orbitron(regularWeight).withHeight(14.0f));
        g.drawFittedText(text, r, juce::Justification::centredLeft, 1);
    }

    void drawPopupMenuSectionHeader(juce::Graphics& g, const juce::Rectangle<int>& area,
                                    const juce::String& sectionName) override
    {
        g.setColour(juce::Colour(0xff00ff41));
        g.setFont(orbitron(boldWeight).withHeight(14.0f));
        g.drawFittedText(sectionName, area.reduced(10, 0), juce::Justification::centredLeft, 1);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                              bool isHighlighted, bool isDown) override
    {
        const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);

        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour((isHighlighted || isDown) && button.isEnabled() ? juce::Colour(0xff00ff41) : juce::Colour(0xff3a3a3a));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool isHighlighted, bool isDown) override
    {
        auto colour = juce::Colours::white;

        if (! button.isEnabled())
            colour = juce::Colour(0xff666666);
        else if (isHighlighted || isDown)
            colour = juce::Colour(0xff00ff41);

        g.setColour(colour);
        g.setFont(getTextButtonFont(button, button.getHeight()));
        g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(4, 0),
                         juce::Justification::centred, 1);
    }

    juce::Font getTextButtonFont(juce::TextButton&, int) override
    {
        return orbitron(regularWeight).withHeight(14.0f);
    }
    
    juce::Font getLabelFont(juce::Label& label) override
    {
        const auto existing = label.getFont();
        const auto options  = orbitron(existing.isBold() ? boldWeight : regularWeight);
        return options.withHeight(existing.getHeight());
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return orbitron(regularWeight).withHeight(14.0f);
    }

    juce::Font getSliderPopupFont(juce::Slider&) override
    {
        return orbitron(regularWeight).withHeight(12.0f);
    }
    
    juce::Label* createComboBoxTextBox(juce::ComboBox&) override
    {
        auto* label = new juce::Label();
        label->setJustificationType(juce::Justification::centredLeft);
        label->setColour(juce::Label::textColourId, juce::Colours::white);
        label->setColour(juce::Label::textWhenEditingColourId, juce::Colours::white);
        label->setFont(orbitron(regularWeight).withHeight(14.0f));
        return label;
    }
    
    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g, bool /*isMouseOver*/, bool /*isMouseDown*/) override
    {
        auto bounds = button.getActiveArea().toFloat();
        auto neonGreen = juce::Colour(0xff00ff41);
        
        // Background
        if (button.getToggleState())
        {
            g.setColour(juce::Colour(0xff2a2a2a));
            g.fillRect(bounds);
        }
        else
        {
            g.setColour(juce::Colour(0xff1a1a1a));
            g.fillRect(bounds);
        }
        
        // Bottom indicator for active tab
        if (button.getToggleState())
        {
            g.setColour(neonGreen);
            g.fillRect(bounds.removeFromBottom(3.0f));
        }
        
        // Text
        g.setColour(button.getToggleState() ? neonGreen : juce::Colour(0xff666666));
        g.setFont(orbitron(boldWeight).withHeight(14.0f));
        g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
    }
    
    int getTabButtonBestWidth(juce::TabBarButton& button, int tabDepth) override
    {
        juce::ignoreUnused(tabDepth);

        auto* tabBar = button.findParentComponentOfClass<juce::TabbedButtonBar>();
        return tabBar->getWidth() / tabBar->getNumTabs();
    }
    
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPosProportional, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/,
                         juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centerX = bounds.getCentreX();
        auto centerY = bounds.getCentreY();
        
        auto rotaryStartAngle = juce::MathConstants<float>::pi * 1.25f; 
        auto rotaryEndAngle = juce::MathConstants<float>::pi * 2.75f;   
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        
        auto neonGreen = juce::Colour(0xff00ff41);
        auto trackColor = juce::Colour(0xff3a3a3a);
        
        // Draw background circle
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillEllipse(bounds.reduced(2.0f));
        
        // Draw track
        auto arcRadius = radius - 5.0f;
        auto lineThickness = 3.0f;
        
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centerX, centerY, arcRadius, arcRadius,
                                    0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(trackColor);
        g.strokePath(backgroundArc, juce::PathStrokeType(lineThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        // Draw value arc
        if (sliderPosProportional > 0.0f)
        {
            juce::Path valueArc;
            valueArc.addCentredArc(centerX, centerY, arcRadius, arcRadius,
                                   0.0f, rotaryStartAngle, angle, true);
            g.setColour(slider.isEnabled() ? neonGreen : juce::Colour(0xff666666));
            g.strokePath(valueArc, juce::PathStrokeType(lineThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        
        // Draw pointer notch
        auto notchLength = radius * 0.3f;
        auto notchWidth = 2.5f;
        auto notchDistance = radius * 0.55f;
        
        juce::Path notch;
        notch.addRoundedRectangle(-notchWidth * 0.5f, -notchDistance,
                                  notchWidth, notchLength, 1.0f);
        notch.applyTransform(juce::AffineTransform::rotation(angle).translated(centerX, centerY));
        
        g.setColour(slider.isEnabled() ? juce::Colours::white : juce::Colour(0xff888888));
        g.fillPath(notch);
    }

};
