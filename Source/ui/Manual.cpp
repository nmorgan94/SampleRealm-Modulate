#include "Manual.h"
#include "CustomLookAndFeel.h"
#include <BinaryData.h>

namespace Manual
{
    static std::optional<Block> startOf (const juce::String& line)
    {
        for (const auto& [prefix, kind] : { std::pair { "## ", Block::Kind::subheading },
                                            std::pair { "# ", Block::Kind::heading },
                                            std::pair { "- ", Block::Kind::bullet } })
            if (line.startsWith (prefix))
                return Block { kind, line.fromFirstOccurrenceOf (prefix, false, false).trim() };

        return std::nullopt;
    }

    std::vector<Block> parse (const juce::String& markdown)
    {
        std::vector<Block> blocks;
        bool canContinue = false;

        for (const auto& rawLine : juce::StringArray::fromLines (markdown))
        {
            const auto line = rawLine.trim();

            if (line.isEmpty())
            {
                canContinue = false;
            }
            else if (auto block = startOf (line))
            {
                canContinue = block->kind == Block::Kind::bullet;
                blocks.push_back (std::move (*block));
            }
            else if (canContinue)
            {
                blocks.back().text << ' ' << line;
            }
            else
            {
                blocks.push_back ({ Block::Kind::paragraph, line });
                canContinue = true;
            }
        }

        return blocks;
    }

    juce::String text()
    {
        return juce::String::createStringFromData (BinaryData::Manual_md, BinaryData::Manual_mdSize);
    }
}

//==============================================================================
class ManualOverlay::Page : public juce::Component
{
public:
    explicit Page (std::vector<Manual::Block> b) : blocks (std::move (b)) {}

    /** Wraps the text to the width, and takes the height it needs. */
    void layOut (int width)
    {
        lines.clear();
        colour = findColour (juce::Label::textColourId);
        float y = 0.0f;

        for (const auto& block : blocks)
        {
            const auto isBullet = block.kind == Manual::Block::Kind::bullet;
            const auto [height, weight, gapAbove] = style (block.kind);

            if (! lines.empty())
                y += gapAbove;

            juce::AttributedString text;
            text.append (block.text, juce::Font (CustomLookAndFeel::orbitron (weight).withHeight (height)), colour);
            text.setWordWrap (juce::AttributedString::byWord);

            const auto x = isBullet ? bulletIndent : 0.0f;
            juce::TextLayout layout;
            layout.createLayout (text, static_cast<float> (width) - x);

            const juce::Rectangle<float> bounds { x, y, layout.getWidth(), layout.getHeight() };
            lines.push_back ({ std::move (layout), bounds, isBullet });
            y = bounds.getBottom();
        }

        setSize (width, static_cast<int> (std::ceil (y)));
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (colour);

        for (const auto& line : lines)
        {
            line.layout.draw (g, line.bounds);

            // Orbitron has no bullet glyph
            if (line.isBullet)
            {
                const auto centreY = line.bounds.getY() + line.layout.getLine (0).getLineBounds().getCentreY();
                g.fillEllipse (4.0f, centreY - 2.0f, 4.0f, 4.0f);
            }
        }
    }

private:
    static constexpr float bulletIndent = 14.0f;

    struct Line
    {
        juce::TextLayout layout;
        juce::Rectangle<float> bounds;
        bool isBullet;
    };

    struct Style { float height, weight, gapAbove; };

    static Style style (Manual::Block::Kind kind)
    {
        switch (kind)
        {
            case Manual::Block::Kind::heading:      return { 16.0f, CustomLookAndFeel::boldWeight, 14.0f };
            case Manual::Block::Kind::subheading:   return { 14.0f, CustomLookAndFeel::boldWeight, 14.0f };
            case Manual::Block::Kind::bullet:       return { 12.0f, CustomLookAndFeel::regularWeight, 4.0f };
            case Manual::Block::Kind::paragraph:    break;
        }

        return { 12.0f, CustomLookAndFeel::regularWeight, 8.0f };
    }

    std::vector<Manual::Block> blocks;
    std::vector<Line> lines;
    juce::Colour colour;
};

//==============================================================================
ManualOverlay::ManualOverlay (const juce::String& markdown) : page (std::make_unique<Page> (Manual::parse (markdown)))
{
    titleLabel.setText ("Manual", juce::dontSendNotification);
    closeButton.onClick = [this] { dismiss(); };

    viewport.setViewedComponent (page.get(), false);
    viewport.setScrollBarsShown (true, false);

    addAndMakeVisible (closeButton);
    addAndMakeVisible (viewport);
}

ManualOverlay::~ManualOverlay() = default;

void ManualOverlay::show()
{
    open();
    layOutPage();
    viewport.setViewPosition (0, 0);
}

void ManualOverlay::layOutPage()
{
    page->layOut (viewport.getWidth() - viewport.getScrollBarThickness() - 4);
}

void ManualOverlay::resized()
{
    auto area = getPanelBounds().reduced (16, 12);

    auto top = area.removeFromTop (titleHeight);
    closeButton.setBounds (top.removeFromRight (70));
    titleLabel.setBounds (top);
    area.removeFromTop (8);

    viewport.setBounds (area);

    // Not while hidden: an editor that never opens the manual never lays it out
    if (isVisible())
        layOutPage();
}
