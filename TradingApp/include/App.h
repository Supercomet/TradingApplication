#pragma once

#include <fstream>
#include <unordered_map>
#include <vector>
#include "imgui.h"

struct DataFrame
{
	uint64_t date{};
	double open{};
	double close{};
	double high{};
	double low{};
	double volume{};

	void Print();
};


class App
{
public:
	void Run();

private:
	std::unordered_map<std::string, std::vector<DataFrame>> dataMap;

	void ParseFile(const char *fileName);
	bool ParseLine(std::fstream& file, DataFrame& outFrame, std::string& outName);

	void ShowTraderWindow();
	void PlotCandlestick(const char* label_id, const double* xs, const double* opens, const double* closes, const double* lows, const double* highs, int count, bool tooltip, float width_percent, ImVec4 bullCol, ImVec4 bearCol);
};

