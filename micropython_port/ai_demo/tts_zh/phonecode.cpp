
#include "phonecode.h"
#include "num.h"
#include "pinyin_utils.h"


/*规范化固话/手机号码
# 手机
# http://www.jihaoba.com/news/show/13680
# 移动：139、138、137、136、135、134、159、158、157、150、151、152、188、187、182、183、184、178、198
# 联通：130、131、132、156、155、186、185、176
# 电信：133、153、189、180、181、177 
因为regex不支持?<!，所以只能使用boost/xpressive/xpressive实现
*/




std::string phone2str_zh(std::string phone_string, bool mobile = true) {
    if (mobile) {
        //去除首尾的+
        
        phone_string = phone_string.substr( phone_string.find_first_not_of('+'));
        phone_string = phone_string.substr(0, phone_string.find_last_not_of('+') + 1);
        std::vector<std::string> sp_parts = split(phone_string, ' ');
       
        string result;
        for (auto part : sp_parts) {
            if(!result.empty())
                result+="，";
            
            result += verbalize_digit(part, true);
            
        }
        return result;
    } 
    else {
        std::vector<std::string> sil_parts = split(phone_string, '-');
        string result;
        for (auto part : sil_parts) {
            if(!result.empty())
                result+="，";
            result += verbalize_digit(part, true);
        }
        return result;
    }
}

std::string phone2str_zh_(std::string phone_string, bool mobile = true) {
    if (mobile) {
        //去除首尾的+
        
        phone_string = phone_string.substr( phone_string.find_first_not_of('+'));
        phone_string = phone_string.substr(0, phone_string.find_last_not_of('+') + 1);
        std::vector<std::string> sp_parts = split(phone_string, '-');
       
        string result;
        for (auto part : sp_parts) {
            if(!result.empty())
                result+="，";
            
            result += verbalize_digit(part, true);
            
        }
        return result;
    } 
   
}


std::string replace_phone_zh(std::smatch match) {
    return phone2str_zh(match[0], false);
}


std::string replace_mobile_zh(std::smatch match) {
    return phone2str_zh(match[0]);
}

std::string replace_mobile_zh_(std::smatch match) {
    return phone2str_zh_(match[0]);
}


//查找并替换
string replace_phonecode_zh(ppf p,string text,regex e){
    string s = text;
    string result="";
    std::smatch m;
    // boost::xpressive::smatch m;
    while (regex_search(s, m, e)) {
        result+=m.prefix();
       //因为regex不能使用?<! 所以在匹配后判断匹配结果前的是否是数字，如果是数字则不匹配
        if((result.back()=='0')|(result.back()=='1')|(result.back()=='2')|(result.back()=='3')|(result.back()=='4')|(result.back()=='5')|(result.back()=='6')|(result.back()=='7')
        |(result.back()=='8')|(result.back()=='9'))
        {
            result+=m[0].str();
        }
        else
        {
             //替换
            string replace_str = p(m);
            result+=replace_str;
        }
        
        
        s = m.suffix().str();  // 返回末端，作为新的搜索的开始
    }
    if(!s.empty())
        result+=s;
    return result;
}



