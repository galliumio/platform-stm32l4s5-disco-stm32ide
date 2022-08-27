/*******************************************************************************
 * Copyright (C) 2018 Gallium Studio LLC (Lawrence Lo). All rights reserved.
 *
 * This program is open source software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Alternatively, this program may be distributed and modified under the
 * terms of Gallium Studio LLC commercial licenses, which expressly supersede
 * the GNU General Public License and are specifically designed for licensees
 * interested in retaining the proprietary status of their code.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Contact information:
 * Website - https://www.galliumstudio.com
 * Source repository - https://github.com/galliumstudio
 * Email - admin@galliumstudio.com
 ******************************************************************************/


#include "fw_log.h"
#include "fw_assert.h"
#include "WM.h"
#include "LCDConf_Lin_Template.h"
#include "GuiText.h"

FW_DEFINE_THIS_FILE("GuiText.cpp")

namespace APP {

#define TOP_PADDING         1   // pixel
#define BOTTOM_PADDING      7

WM_HWIN GuiText::Create(Active *owner, WM_HWIN hParent, int xPos, int yPos,
                          int xSize, int ySize, GUI_COLOR borColor, WM_CALLBACK *cb)
{
    FW_ASSERT(hParent && xSize && ySize && cb);
    FW_ASSERT(owner);
    FW_ASSERT(m_hWin == 0);
    
    m_hsmn = owner->GetHsmn();       // For logging.
    m_xPos = xPos;
    m_yPos = yPos;
    m_xSize = xSize;
    m_ySize = ySize;
    m_hParent = hParent;
    m_hWin = WM_CreateWindowAsChild(xPos, yPos, xSize, ySize, hParent, WM_CF_SHOW | WM_CF_LATE_CLIP, cb, sizeof(owner));
    FW_ASSERT(m_hWin);
    WM_SetUserData(m_hWin, &owner, sizeof(owner));   
    
    m_bufIdx = 0;
    memset(m_textBuf, 0, sizeof(m_textBuf));
    memset(m_ySizeText, 0, sizeof(m_ySizeText));  
    uint32_t i;
    for (i=0; i<BUF_CNT; i++)
    {
        m_pFont[i] = GUI_FONT_8_ASCII;
        m_textColor[i] = GUI_BLACK;
        m_bgColor[i] = GUI_BLACK;
    }
    m_borColor = borColor;    
    m_vxPos = 1;
    m_vyPos = 1;        
    return m_hWin;
}

void GuiText::SetText(uint32_t bufIdx, const char *pText, const GUI_FONT *pFont, 
                      GUI_COLOR textColor, GUI_COLOR bgColor)
{
    FW_ASSERT(pText);
    FW_ASSERT(m_hWin);
    FW_ASSERT(bufIdx < m_bufCnt);

    strncpy(m_textBuf[bufIdx], pText, MAX_TEXT_LEN-1);
    m_textBuf[bufIdx][MAX_TEXT_LEN-1] = 0;
    m_pFont[bufIdx] = pFont;
    m_textColor[bufIdx] = textColor;
    m_bgColor[bufIdx] = bgColor;
    GUI_SetFont(pFont);    
    // -4 to account for border on each side
    FW_ASSERT(m_xSize > 4);
    m_ySizeText[bufIdx] = GUI_GetFontSizeY() *GUI_WrapGetNumLines(m_textBuf[bufIdx], m_xSize - 4, GUI_WRAPMODE_WORD) +
                          + TOP_PADDING + BOTTOM_PADDING; 
    if (m_ySizeText[bufIdx] < (m_ySize - 2))
    {
        m_ySizeText[bufIdx] = m_ySize - 2;
    }    
    WM_Invalidate(m_hWin);           
}

void GuiText::Update(int dy, uint32_t& bufIdx, uint32_t& offsetLeft, uint32_t& offsetRight)
{
    FW_ASSERT(m_hWin);
    FW_ASSERT(dy < 0);

    if (((m_ySizeText[m_bufIdx] - BOTTOM_PADDING) <= (m_ySize - 2)) && (GetNextBufIdx() == m_bufIdx))
    {
        // no need to scroll, reset
        m_vyPos = 1;
    }
    else
    {
        m_vyPos += dy;
        if ((m_vyPos + m_ySizeText[m_bufIdx]) <= 1)
        {
            m_vyPos = 1;
            m_bufIdx = GetNextBufIdx();
        }
    }
    bufIdx = m_bufIdx;
    offsetLeft = 1 - m_vyPos;
    offsetRight = m_vyPos + m_ySizeText[m_bufIdx] - 1;    
    WM_Invalidate(m_hWin);        
}

Area GuiText::Paint()
{
    FW_ASSERT(m_hWin);
    WM_SelectWindow(m_hWin);
    FW_ASSERT(m_bufIdx < m_bufCnt);
    GUI_SetBkColor(m_bgColor[m_bufIdx]);
    GUI_Clear();
    GUI_SetTextMode(GUI_TM_TRANS);
    GUI_SetFont(m_pFont[m_bufIdx]);
    GUI_SetColor(m_textColor[m_bufIdx]);
    GUI_RECT rect;
    rect.x0 = m_vxPos + 1;
    rect.y0 = m_vyPos + TOP_PADDING;
    rect.x1 = rect.x0 + (m_xSize - 4) - 1;
    rect.y1 = rect.y0 + (m_ySizeText[m_bufIdx] - TOP_PADDING) - 1;
    GUI_DispStringInRectWrap(m_textBuf[m_bufIdx], &rect, /*GUI_TA_TOP | GUI_TA_LEFT*/ GUI_TA_VCENTER | GUI_TA_HCENTER, GUI_WRAPMODE_WORD);
    uint32_t nextIdx = GetNextBufIdx();
    FW_ASSERT(nextIdx < m_bufCnt);
    GUI_SetFont(m_pFont[nextIdx]);
    GUI_SetColor(m_bgColor[nextIdx]);
    GUI_FillRect(m_vxPos, m_vyPos + m_ySizeText[m_bufIdx], m_xSize-2, m_ySize-2);
    GUI_SetColor(m_textColor[nextIdx]);
    rect.x0 = m_vxPos + 1;
    rect.y0 = m_vyPos + m_ySizeText[m_bufIdx] + TOP_PADDING;
    rect.x1 = rect.x0 + (m_xSize - 4) - 1;
    rect.y1 = rect.y0 + (m_ySizeText[nextIdx] - TOP_PADDING) - 1;
    GUI_DispStringInRectWrap(m_textBuf[nextIdx], &rect, /*GUI_TA_TOP | GUI_TA_LEFT*/ GUI_TA_VCENTER | GUI_TA_HCENTER, GUI_WRAPMODE_WORD);
    GUI_SetColor(m_borColor);   
    GUI_DrawRoundedFrame(0, 0, m_xSize - 1, m_ySize - 1, 0, 1);
    return GetArea();
}
}
