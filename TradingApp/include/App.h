#pragma once

#include "imgui.h"
class App
{
public:
	void Run();

private:
	void ShowTraderWindow();
	void PlotCandlestick(const char* label_id, const double* xs, const double* opens, const double* closes, const double* lows, const double* highs, int count, bool tooltip, float width_percent, ImVec4 bullCol, ImVec4 bearCol);
};

