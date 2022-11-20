#include "CText.h"
#include <windef.h>
using namespace std;
CText::CText() {}
CText::~CText() {
}

bool CText::create(IDirect3DDevice9* pDevice, int screenWidth, int screenHeight, string input) {
	this->text = input;
	rct.left = 0;
	rct.right = screenWidth;
	rct.top = 0;
	rct.bottom = screenHeight;
	fontColor = D3DCOLOR_ARGB(255, 255, 255, 255);
	if (FAILED(D3DXCreateFont(pDevice, 25, 12, 500, 0, false, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, 0, 0, "Comic Sans MS", &ppFont))) {
		return false;
	}
	anchor = DT_LEFT | DT_TOP;
	return true;
}
void CText::draw() {
	ppFont->DrawTextA(0, text.c_str(), -1, &rct, anchor, fontColor);
}
void CText::setPosition(int x, int y) {
	rct.left = x;
	rct.top = y;
}
bool CText::create(IDirect3DDevice9* pDevice, int screenWidth, int screenHeight) {
	rct.left = 512;
	rct.right = screenWidth;
	rct.top = 384;
	rct.bottom = screenHeight;
	if (FAILED(D3DXCreateFont(pDevice, 25, 12, 500, 0, false, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, 0, 0, "Times New Roman", &ppFont))) {
		return false;
	}
	return true;
}
void CText::draw(LPCSTR pString, float x, float y) {
	rct.left = x;
	rct.top = y;
	D3DCOLOR fontColor = D3DCOLOR_ARGB(255, 255, 255, 255);
	ppFont->DrawTextA(0, pString, -1, &rct, DT_LEFT | DT_TOP, fontColor);
}
void CText::destroy() {
	if (ppFont != NULL) {
		ppFont->Release();
		ppFont = NULL;
	}
}