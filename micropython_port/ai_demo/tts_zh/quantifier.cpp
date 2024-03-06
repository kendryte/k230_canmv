#include "quantifier.h"


std::string replace_temperature(smatch match) {
    std::string sign = match[1].str();
    std::string temperature = match[2].str();
    std::string unit = match[3].str();
    string result;
    if (sign == "-") {
        sign = "零下";
    }
    temperature = num2str(temperature);
    if ((unit == "摄氏度")|(unit == "℃")|(unit == "°C")){
        unit = "摄氏度";
    }
    else
    {
        unit = "度";
    }
    
    result = sign + temperature + unit;
    return result;
}