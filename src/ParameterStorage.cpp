#include "ParameterStorage.h"

/// @brief 製造番号をFRAMから読み出す（char配列)
/// @param parameter_name パラメータ名(enum)
/// @param parameter 製造番号
/// @return True:成功   False:失敗
bool ParameterStorage::read(const E_ParameterCategories parameter_name, char* parameter){
    if (DEBUG){Serial.print("Method-read: ");}
    if (parameter_name == E_ParameterCategories::SERIAL_NUMBER){
        const uint16_t idx = static_cast<uint16_t>(parameter_name);            
        size_t i = 0;
        for (i=0; i < SER_NUM_MAX_LENGTH; i++){
            parameter[i] = Adafruit_FRAM_I2C::read8(idx + i);
            if (parameter[i] == '\0') break;
        }
        if (i == SER_NUM_MAX_LENGTH | i == 0){
            if (DEBUG){Serial.println("found no EOL!");}
            return false;
        }
        if (DEBUG){Serial.println("finish reading.");}
        return true;
    };
    return false;
};


/// @brief 製造番号をFRAMに書き込む（char配列)
/// @param parameter_name パラメータ名(enum)
/// @param parameter 製造番号
/// @param len 配列の長さ<=16
/// @return True:成功   False:失敗
bool ParameterStorage::store(const E_ParameterCategories parameter_name, const char* parameter, size_t len){
    if (DEBUG){Serial.print("Method-store: ");}
    if (parameter_name == E_ParameterCategories::SERIAL_NUMBER){
        const uint16_t idx = static_cast<uint16_t>(parameter_name);
        // size_t len = sizeof(ser_num);
        if (DEBUG) {Serial.println(len);}

        // 配列の長さチェック
        if (len > SER_NUM_MAX_LENGTH){
            if (DEBUG) {Serial.println("too long");}
            return false;
        }

        //  char配列の整合性チェック  最後にNULLのないもの、文字の入っていないNULL配列を排除
        if (parameter[len-1]!='\0' | parameter[0]=='\0'){
            if (DEBUG) {Serial.println("invalid char array");}
            return false;
        }

        for (uint16_t i = 0; i < len ; i++){
            if (DEBUG) {Serial.print(parameter[i]);}
            Adafruit_FRAM_I2C::write8(idx + i, parameter[i]);
        }
        
        if (DEBUG) {Serial.println("finish sotre.");}
        
        return true;
    }
    return false;
};

