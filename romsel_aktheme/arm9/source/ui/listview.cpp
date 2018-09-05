/*
    listview.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ui.h"
#include "listview.h"
#include "drawing/gdi.h"
#include "tool/dbgtool.h"

namespace akui
{

void ListItem::setText(const std::string &text)
{
    _text = text;
    _lines = 1;
    size_t pos = _text.find('\n');
    while (_text.npos != pos)
    {
        ++_lines;
        pos = _text.find('\n', pos + 1);
    }
}

ListView::ListView(s32 x, s32 y, u32 w, u32 h, Window *parent, const std::string &text, int scrollSpeed)
    : Window(parent, text), _scrollSpeed(scrollSpeed)
{
    _size = Size(w, h);
    _position = Point(x, y);
    _rowHeight = 12;
    _selectedRowId = 0;
    _firstVisibleRowId = 0;
    _visibleRowCount = 0;
    _rowsPerpage = 0;
    _textColor = uiSettings().listTextColor;                        //RGB15(24,24,24);
    _textColorHilight = uiSettings().listTextHighLightColor;        //RGB15(31,31,31);
    _selectionBarColor1 = BIT(15) | uiSettings().listViewBarColor1; //RGB15(0,0,31);
    _selectionBarColor2 = BIT(15) | uiSettings().listViewBarColor2; //RGB15(0,0,31);
    _selectionBarOpacity = 100;
    _engine = GE_MAIN;
    _scrollSpeed = scrollSpeed;
    _touchMovedAfterTouchDown = false;
}

void ListView::arangeColumnsSize()
{
    u16 offset = 0;
    for (size_t i = 0; i < _columns.size(); ++i)
    {
        _columns[i].offset = offset;
        offset += _columns[i].width;
    }
}

bool ListView::insertColumn(size_t index, const std::string &text, u8 width)
{
    if (index > _columns.size())
        return false;

    ListColumn aColumn;
    aColumn.width = width;
    if (index > 0)
        aColumn.offset = _columns[index - 1].offset + _columns[index - 1].width;
    else
        aColumn.offset = 0;

    _columns.insert(_columns.begin() + index, aColumn);
    return true;
}

bool ListView::insertRow(size_t index, const std::vector<std::string> &texts)
{
    size_t columnCount = _columns.size();
    if (0 == columnCount || index > _rows.size())
        return false;
    if (0 == texts.size())
        return false;

    _rows.insert(_rows.begin() + index, itemVector());
    for (size_t col = 0; col < columnCount; ++col)
    {
        std::string itemText;
        if (col >= texts.size())
            itemText = "Empty"; // Ĭ���ַ���
        else
            itemText = texts[col];

        ListItem aItem;
        aItem.setText(itemText);

        _rows[index].insert(_rows[index].begin() + col, aItem);
    }
    //if( _visibleRowCount > _rows.size() ) _visibleRowCount = _rows.size();

    return true;
}

void ListView::removeRow(size_t index)
{
    if (index >= _rows.size())
        return;
    _rows.erase(_rows.begin() + index);
}

void ListView::removeAllRows()
{
    _rows.clear();
    _selectedRowId = 0;
    _firstVisibleRowId = 0;
    //_visibleRowCount = 0;
}

void ListView::draw()
{
    //dbg_printf( "cListView::draw %08x\n", this );
    //draw_frame();
    drawSelectionBar();
    drawText();
}

void ListView::drawSelectionBar()
{
    //if( _touchMovedAfterTouchDown )
    //    return;

    s16 x = _position.x - 2;
    s16 y = _position.y + (_selectedRowId - _firstVisibleRowId) * _rowHeight - 1;
    s16 w = _size.x + 4;
    s16 h = _rowHeight;

    //gdi().setPenColor( _selectionBarColor );
    for (u8 i = 0; i < h; ++i)
    {
        if (i & 1)
            gdi().fillRectBlend(_selectionBarColor1, _selectionBarColor2, x, y + i, w, 1, _engine, _selectionBarOpacity);
        else
            gdi().fillRectBlend(_selectionBarColor2, _selectionBarColor1, x, y + i, w, 1, _engine, _selectionBarOpacity);
    }
}

void ListView::drawText()
{
    size_t columnCount = _columns.size();
    size_t total = _visibleRowCount;
    if (total > _rows.size() - _firstVisibleRowId)
        total = _rows.size() - _firstVisibleRowId;

    for (size_t i = 0; i < total; ++i)
    {
        for (size_t j = 0; j < columnCount; ++j)
        {
            s32 height = _rows[_firstVisibleRowId + i][j].lines() * SYSTEM_FONT_HEIGHT;
            s32 itemX = _position.x + _columns[j].offset;
            s32 itemY = _position.y + i * _rowHeight;
            s32 textY = itemY + ((_rowHeight - height - 1) >> 1);
            if (textY + height > (s32)(_position.y + _size.y))
                break;
            if (_selectedRowId == _firstVisibleRowId + i /* && !_touchMovedAfterTouchDown */)
                gdi().setPenColor(_textColorHilight, _engine);
            else
                gdi().setPenColor(_textColor, _engine);
            if (ownerDraw.size())
            {
                ownerDraw(OwnerDraw(_firstVisibleRowId + i, j, Point(itemX, itemY - 1), Size(_columns[j].width, _rowHeight), textY, height, _rows[_firstVisibleRowId + i][j].text().c_str(), _engine));
            }
            else
            {
                gdi().textOutRect(itemX, textY, _columns[j].width, height, _rows[_firstVisibleRowId + i][j].text().c_str(), _engine);
            }
        }
    }
    //dbg_printf( "total %d _visible_row_count %d\n", total, _visible_row_count );
}

void ListView::selectRow(int id)
{
    if (0 == _rows.size())
        return;

    if (id > (int)_rows.size() - 1)
        id = (int)_rows.size() - 1;
    if (id < 0)
        id = 0;

    //if( (int)_selectedRowId == id ) return;

    size_t lastVisibleRowId = _firstVisibleRowId + _visibleRowCount - 1;
    if (lastVisibleRowId > _rows.size() - 1)
        lastVisibleRowId = _rows.size() - 1;
    if (id < (int)_firstVisibleRowId)
    {
        int offset = _selectedRowId - _firstVisibleRowId;
        scrollTo(id - offset);
    }
    if (id > (int)lastVisibleRowId)
    {
        int offset = _selectedRowId - _firstVisibleRowId;
        scrollTo(id - offset);
    }
    if (_selectedRowId != (u32)id)
    {
        _selectedRowId = id;
        onSelectChanged(_selectedRowId);
        selectChanged(_selectedRowId);
    }
}

void ListView::setFirstVisibleIdAndSelectRow(u32 first, u32 row)
{
    if (0 == _rows.size())
        return;
    if (first >= _rows.size())
        return;
    _firstVisibleRowId = first;
    selectRow(row);
}

void ListView::scrollTo(int id)
{
    if (0 == _rows.size())
        return;
    //if( _rows.size() < _visibleRowCount ) return;
    //dbg_printf("rows size %d, visibleRowCount %d\n", _rows.size(), _visibleRowCount );

    if (id > (int)_rows.size() - 1)
        id = (int)_rows.size() - 1;
    if (id > (int)_rows.size() - (int)_visibleRowCount)
        id = (int)_rows.size() - (int)_visibleRowCount;
    if (id < 0)
        id = 0;
    _firstVisibleRowId = id;
    onScrolled(id);
    scrolled(id);
    //dbg_printf("fvri %d, scroll_to %d\n", _first_visible_row_id, id );
}

Window &ListView::loadAppearance(const std::string &aFileName)
{
    return *this;
}

u32 ListView::rowBelowPoint(const Point &pt)
{
    if (windowRectangle().surrounds(pt))
    {
        u32 row = _firstVisibleRowId + (pt.y - position().y) / _rowHeight;
        if (row > _rows.size() - 1)
            row = (u32)-1;
        return row;
    }
    return (u32)-1;
}

bool ListView::process(const Message &msg)
{
    bool ret = false;
    if (isVisible())
    {
        //if( msg.id() > Message::keyMessageStart
        //    && msg.id() < Message::keyMessageEnd )
        //{
        //    ret = processKeyMessage( (KeyMessage &)msg );
        //}
        if (msg.id() > Message::touchMessageStart && msg.id() < Message::touchMessageEnd)
        {
            ret = processTouchMessage((TouchMessage &)msg);
        }
    }

    return ret;
}

bool ListView::processTouchMessage(const TouchMessage &msg)
{
    bool ret = false;

    static int sumOfMoveY = 0;

    if (msg.id() == Message::touchMove && isFocused())
    {
        if (abs(msg.position().y) > 0)
        {
            sumOfMoveY += msg.position().y;
        }
        if (sumOfMoveY > _scrollSpeed)
        {
            selectNext();
            scrollTo(_firstVisibleRowId + 1);
            sumOfMoveY = 0;
            _touchMovedAfterTouchDown = true;
        }
        else if (sumOfMoveY < -_scrollSpeed)
        {
            selectPrev();
            scrollTo(_firstVisibleRowId - 1);
            sumOfMoveY = 0;
            _touchMovedAfterTouchDown = true;
        }
        ret = true;
    }

    if (msg.id() == Message::touchUp)
    {
        sumOfMoveY = 0;
        if (!_touchMovedAfterTouchDown)
        {
            u32 rbp = rowBelowPoint(Point(msg.position().x, msg.position().y));
            if (rbp != (u32)-1)
            {
                if (selectedRowId() == rbp)
                {
                    onSelectedRowClicked(rbp);
                    selectedRowClicked(rbp);
                }
            }
        }
        _touchMovedAfterTouchDown = false;
        ret = true;
    }

    if (msg.id() == Message::touchDown)
    {
        _touchMovedAfterTouchDown = false;
        u32 rbp = rowBelowPoint(Point(msg.position().x, msg.position().y));
        if (rbp != (u32)-1)
        {
            selectRow(rbp);
        }
        ret = true;
    }

    return ret;
}

void ListView::setScrollSpeed(int scrollSpeed) {
    _scrollSpeed = scrollSpeed;
}

} // namespace akui
