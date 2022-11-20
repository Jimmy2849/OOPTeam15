#pragma once
#include <d3dx9.h>
#include <string>


class CText {
public:
	CText();
	~CText();
	bool create(IDirect3DDevice9* pDevice, int screenWidth, int screenHeight, std::string text);
	void draw();
	inline void setAnchor(DWORD anchor) { this->anchor = anchor; }
	void setPosition(int x, int y);
	inline void setColor(D3DXCOLOR color) { this->fontColor = color; }
	inline void setStr(std::string input) { text.assign(input); }
	void destroy();
private:
	std::string text;
	D3DCOLOR fontColor;
	DWORD anchor;
	bool create(IDirect3DDevice9* pDevice, int screenWidth, int screenHeight);
	void draw(LPCSTR pString, float x, float y);
private:
	LPD3DXFONT ppFont;
	RECT rct;
};