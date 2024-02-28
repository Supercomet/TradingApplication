#pragma once

#include <fstream>
#include <unordered_map>
#include <map>
#include <vector>
#include "imgui.h"

struct DataFrame
{
	double date{};
	double open{};
	double close{};
	double high{};
	double low{};
	double volume{};

	void Print();
};

struct DataStore
{
	std::string name;
	std::vector<double> date;
	std::vector<double> open;
	std::vector<double> close;
	std::vector<double> high;
	std::vector<double> low;
	std::vector<double> volume;

	double maximum{-DBL_MAX };
	double minimum{ DBL_MAX };

	void PushData(const DataFrame& df);
	size_t size() { return date.size(); };
};


class App
{
public:
	void Run();

private:
	std::map<std::string, DataStore> dataMap;
	std::vector<const char*> names;

	void ParseFile(const char *fileName);
	bool ParseLine(std::fstream& file, DataFrame& outFrame, std::string& outName);

	void ShowTraderWindow();
	void PlotCandlestick(const char* label_id, const double* xs, const double* opens, const double* closes, const double* lows, const double* highs, int count, bool tooltip, float width_percent, ImVec4 bullCol, ImVec4 bearCol);
};

