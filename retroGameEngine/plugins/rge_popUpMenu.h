#ifndef jpr_RGEX_POPUPMENU_H
#define jpr_RGEX_POPUPMENU_H

#include <cstdint>

namespace jpr
{
namespace popup
{
constexpr int32_t nPatch = 8;

class Menu
{
public:
    Menu();
    Menu(const std::string n);

    Menu &SetTable(int32_t nColumns, int32_t nRows);
    Menu &SetID(int32_t id);
    Menu &Enable(bool b);

    int32_t GetID();
    std::string &GetName();
    bool Enabled();
    bool HasChildren();
    jpr::vi2d GetSize();
    jpr::vi2d &GetCursorPosition();
    Menu &operator[](const std::string &name);
    void Build();
    void DrawSelf(jpr::RetroGameEngine &pge, jpr::Sprite *sprGFX, jpr::vi2d vScreenOffset);
    void ClampCursor();
    void OnUp();
    void OnDown();
    void OnLeft();
    void OnRight();
    Menu *OnConfirm();
    Menu *GetSelectedItem();

protected:
    int32_t nID = -1;
    jpr::vi2d vCellTable = {1, 0};
    std::unordered_map<std::string, size_t> itemPointer;
    std::vector<jpr::popup::Menu> items;
    jpr::vi2d vSizeInPatches = {0, 0};
    jpr::vi2d vCellSize = {0, 0};
    jpr::vi2d vCellPadding = {2, 0};
    jpr::vi2d vCellCursor = {0, 0};
    int32_t nCursorItem = 0;
    int32_t nTopVisibleRow = 0;
    int32_t nTotalRows = 0;
    const jpr::vi2d vPatchSize = {nPatch, nPatch};
    std::string sName;
    jpr::vi2d vCursorPos = {0, 0};
    bool bEnabled = true;
};

class Manager : public jpr::RGEX
{
public:
    Manager();
    void Open(Menu *mo);
    void Close();
    void OnUp();
    void OnDown();
    void OnLeft();
    void OnRight();
    void OnBack();
    Menu *OnConfirm();
    void Draw(jpr::Sprite *sprGFX, jpr::vi2d vScreenOffset);

private:
    std::list<Menu *> panels;
};

} // namespace popup
}; // namespace jpr

#ifdef jpr_RGEX_POPUPMENU
#undef jpr_RGEX_POPUPMENU

namespace jpr
{
namespace popup
{
Menu::Menu()
{
}

Menu::Menu(const std::string n)
{
    sName = n;
}

Menu &Menu::SetTable(int32_t nColumns, int32_t nRows)
{
    vCellTable = {nColumns, nRows};
    return *this;
}

Menu &Menu::SetID(int32_t id)
{
    nID = id;
    return *this;
}

Menu &Menu::Enable(bool b)
{
    bEnabled = b;
    return *this;
}

int32_t Menu::GetID()
{
    return nID;
}

std::string &Menu::GetName()
{
    return sName;
}

bool Menu::Enabled()
{
    return bEnabled;
}

bool Menu::HasChildren()
{
    return !items.empty();
}

jpr::vi2d Menu::GetSize()
{
    return {int32_t(sName.size()), 1};
}

jpr::vi2d &Menu::GetCursorPosition()
{
    return vCursorPos;
}

Menu &Menu::operator[](const std::string &name)
{
    if (itemPointer.count(name) == 0)
    {
        itemPointer[name] = items.size();
        items.push_back(Menu(name));
    }

    return items[itemPointer[name]];
}

void Menu::Build()
{
    // Recursively build all children, so they can determine their size, use
    // that size to indicate cell sizes if this object contains more than
    // one item
    for (auto &m : items)
    {
        if (m.HasChildren())
        {
            m.Build();
        }

        // Longest child name determines cell width
        vCellSize.x = std::max(m.GetSize().x, vCellSize.x);
        vCellSize.y = std::max(m.GetSize().y, vCellSize.y);
    }

    // Adjust size of this object (in patches) if it were rendered as a panel
    vSizeInPatches.x = vCellTable.x * vCellSize.x + (vCellTable.x - 1) * vCellPadding.x + 2;
    vSizeInPatches.y = vCellTable.y * vCellSize.y + (vCellTable.y - 1) * vCellPadding.y + 2;

    // Calculate how many rows this item has to hold
    nTotalRows = (items.size() / vCellTable.x) + (((items.size() % vCellTable.x) > 0) ? 1 : 0);
}

void Menu::DrawSelf(jpr::RetroGameEngine &pge, jpr::Sprite *sprGFX, jpr::vi2d vScreenOffset)
{
    // === Draw Panel

    // Record current Retro mode user is using
    jpr::Retro::Mode currentRetroMode = pge.GetRetroMode();
    pge.SetRetroMode(jpr::Retro::MASK);

    // Draw Panel & Border
    jpr::vi2d vPatchPos = {0, 0};
    for (vPatchPos.x = 0; vPatchPos.x < vSizeInPatches.x; vPatchPos.x++)
    {
        for (vPatchPos.y = 0; vPatchPos.y < vSizeInPatches.y; vPatchPos.y++)
        {
            // Determine position in screen space
            jpr::vi2d vScreenLocation = vPatchPos * nPatch + vScreenOffset;

            // Calculate which patch is needed
            jpr::vi2d vSourcePatch = {0, 0};
            if (vPatchPos.x > 0)
                vSourcePatch.x = 1;
            if (vPatchPos.x == vSizeInPatches.x - 1)
                vSourcePatch.x = 2;
            if (vPatchPos.y > 0)
                vSourcePatch.y = 1;
            if (vPatchPos.y == vSizeInPatches.y - 1)
                vSourcePatch.y = 2;

            // Draw Actual Patch
            pge.DrawPartialSprite(vScreenLocation, sprGFX, vSourcePatch * nPatch, vPatchSize);
        }
    }

    // === Draw Panel Contents
    jpr::vi2d vCell = {0, 0};
    vPatchPos = {1, 1};

    // Work out visible items
    int32_t nTopLeftItem = nTopVisibleRow * vCellTable.x;
    int32_t nBottomRightItem = vCellTable.y * vCellTable.x + nTopLeftItem;

    // Clamp to size of child item vector
    nBottomRightItem = std::min(int32_t(items.size()), nBottomRightItem);
    int32_t nVisibleItems = nBottomRightItem - nTopLeftItem;

    // Draw Scroll Markers (if required)
    if (nTopVisibleRow > 0)
    {
        vPatchPos = {vSizeInPatches.x - 2, 0};
        jpr::vi2d vScreenLocation = vPatchPos * nPatch + vScreenOffset;
        jpr::vi2d vSourcePatch = {3, 0};
        pge.DrawPartialSprite(vScreenLocation, sprGFX, vSourcePatch * nPatch, vPatchSize);
    }

    if ((nTotalRows - nTopVisibleRow) > vCellTable.y)
    {
        vPatchPos = {vSizeInPatches.x - 2, vSizeInPatches.y - 1};
        jpr::vi2d vScreenLocation = vPatchPos * nPatch + vScreenOffset;
        jpr::vi2d vSourcePatch = {3, 2};
        pge.DrawPartialSprite(vScreenLocation, sprGFX, vSourcePatch * nPatch, vPatchSize);
    }

    // Draw Visible Items
    for (int32_t i = 0; i < nVisibleItems; i++)
    {
        // Cell location
        vCell.x = i % vCellTable.x;
        vCell.y = i / vCellTable.x;

        // Patch location (including border offset and padding)
        vPatchPos.x = vCell.x * (vCellSize.x + vCellPadding.x) + 1;
        vPatchPos.y = vCell.y * (vCellSize.y + vCellPadding.y) + 1;

        // Actual screen location in Retros
        jpr::vi2d vScreenLocation = vPatchPos * nPatch + vScreenOffset;

        // Display Item Header
        pge.DrawString(vScreenLocation, items[nTopLeftItem + i].sName, items[nTopLeftItem + i].bEnabled ? jpr::WHITE : jpr::DARK_GREY);

        if (items[nTopLeftItem + i].HasChildren())
        {
            // Display Indicator that panel has a sub panel
            vPatchPos.x = vCell.x * (vCellSize.x + vCellPadding.x) + 1 + vCellSize.x;
            vPatchPos.y = vCell.y * (vCellSize.y + vCellPadding.y) + 1;
            jpr::vi2d vSourcePatch = {3, 1};
            vScreenLocation = vPatchPos * nPatch + vScreenOffset;
            pge.DrawPartialSprite(vScreenLocation, sprGFX, vSourcePatch * nPatch, vPatchSize);
        }
    }

    // Calculate cursor position in screen space in case system draws it
    vCursorPos.x = (vCellCursor.x * (vCellSize.x + vCellPadding.x)) * nPatch + vScreenOffset.x - nPatch;
    vCursorPos.y = ((vCellCursor.y - nTopVisibleRow) * (vCellSize.y + vCellPadding.y)) * nPatch + vScreenOffset.y + nPatch;
}

void Menu::ClampCursor()
{
    // Find item in children
    nCursorItem = vCellCursor.y * vCellTable.x + vCellCursor.x;

    // Clamp Cursor
    if (nCursorItem >= int32_t(items.size()))
    {
        vCellCursor.y = (items.size() / vCellTable.x);
        vCellCursor.x = (items.size() % vCellTable.x) - 1;
        nCursorItem = items.size() - 1;
    }
}

void Menu::OnUp()
{
    vCellCursor.y--;
    if (vCellCursor.y < 0)
        vCellCursor.y = 0;

    if (vCellCursor.y < nTopVisibleRow)
    {
        nTopVisibleRow--;
        if (nTopVisibleRow < 0)
            nTopVisibleRow = 0;
    }

    ClampCursor();
}

void Menu::OnDown()
{
    vCellCursor.y++;
    if (vCellCursor.y == nTotalRows)
        vCellCursor.y = nTotalRows - 1;

    if (vCellCursor.y > (nTopVisibleRow + vCellTable.y - 1))
    {
        nTopVisibleRow++;
        if (nTopVisibleRow > (nTotalRows - vCellTable.y))
            nTopVisibleRow = nTotalRows - vCellTable.y;
    }

    ClampCursor();
}

void Menu::OnLeft()
{
    vCellCursor.x--;
    if (vCellCursor.x < 0)
        vCellCursor.x = 0;
    ClampCursor();
}

void Menu::OnRight()
{
    vCellCursor.x++;
    if (vCellCursor.x == vCellTable.x)
        vCellCursor.x = vCellTable.x - 1;
    ClampCursor();
}

Menu *Menu::OnConfirm()
{
    // Check if selected item has children
    if (items[nCursorItem].HasChildren())
    {
        return &items[nCursorItem];
    }
    else
        return this;
}

Menu *Menu::GetSelectedItem()
{
    return &items[nCursorItem];
}

// =====================================================================

Manager::Manager()
{
}

void Manager::Open(Menu *mo)
{
    Close();
    panels.push_back(mo);
}

void Manager::Close()
{
    panels.clear();
}

void Manager::OnUp()
{
    if (!panels.empty())
        panels.back()->OnUp();
}

void Manager::OnDown()
{
    if (!panels.empty())
        panels.back()->OnDown();
}

void Manager::OnLeft()
{
    if (!panels.empty())
        panels.back()->OnLeft();
}

void Manager::OnRight()
{
    if (!panels.empty())
        panels.back()->OnRight();
}

void Manager::OnBack()
{
    if (!panels.empty())
        panels.pop_back();
}

Menu *Manager::OnConfirm()
{
    if (panels.empty())
        return nullptr;

    Menu *next = panels.back()->OnConfirm();
    if (next == panels.back())
    {
        if (panels.back()->GetSelectedItem()->Enabled())
            return panels.back()->GetSelectedItem();
    }
    else
    {
        if (next->Enabled())
            panels.push_back(next);
    }

    return nullptr;
}

void Manager::Draw(jpr::Sprite *sprGFX, jpr::vi2d vScreenOffset)
{
    if (panels.empty())
        return;

    // Draw Visible Menu System
    for (auto &p : panels)
    {
        p->DrawSelf(*pge, sprGFX, vScreenOffset);
        vScreenOffset += {10, 10};
    }

    // Draw Cursor
    jpr::Retro::Mode currentRetroMode = pge->GetRetroMode();
    pge->SetRetroMode(jpr::Retro::ALPHA);
    pge->DrawPartialSprite(panels.back()->GetCursorPosition(), sprGFX, jpr::vi2d(4, 0) * nPatch, {nPatch * 2, nPatch * 2});
    pge->SetRetroMode(currentRetroMode);
}
} // namespace popup
}; // namespace jpr

#endif
#endif