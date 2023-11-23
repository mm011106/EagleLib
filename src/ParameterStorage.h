/**************************************************************************/
/*!
 * @file ParameterStorage.h/cpp
 * @brief FRAMに対しパラメタ（変数・構造体）を保存／呼び出しする
 * @author 
 * @date 20231011
 * $Version:    1.0$
 * @par 
 *    
 * 
 */
/**************************************************************************/

#ifndef _PARAMETERSTORAGE_H_
#define _PARAMETERSTORAGE_H_

#include <Arduino.h>
#include <Adafruit_FRAM_I2C.h>

class ParameterStorage : public Adafruit_FRAM_I2C {

    public:
    // consts
    static constexpr size_t SER_NUM_MAX_LENGTH = 16;

    //  保存するパラメタの種類名を定義  要素の値は保存先のアドレス 
    enum class E_ParameterCategories : uint16_t{
        CAL_FLAG = 0x00,    // bool キャリブレーション済み・未実施
        CAL_DATE = 0x10,    // char[15] キャリブレーション実施日

        SENSOR_LENGTH = 0x40,   // uint8_t センサ長[inch]
        TIMER_PERIOD = 0x48,    // uint8_t タイマ周期設定[min]
        SERIAL_NUMBER =0x50,    // char[15]   製造番号
        SCALING = 0x60,         // 構造体 ScalingParameter スケーリング
        CAL_DATA = 0x70         // 構造体 CalData キャリブレーション値
    };
    // instances

    // types,vars
    struct CalData{
        int16_t adc_offset_01;
        int16_t adc_offset_23;
        float_t adc_gain_error_01;
        float_t adc_gain_error_23;
        uint16_t vmon_da_offset;
        uint16_t current_adj;
    };
    struct ScalingParameter{
        uint16_t hi_side_scale;
        uint16_t low_side_scale;
    };

    // methods 
    /*!
    * @brief constructor  
    */
    ParameterStorage(){
    };

    /*!
    * @brief deconstructor
    *  
    */
    ~ParameterStorage(){
    };

    /// @brief parameterをFRAMから読み出す(任意の型 char[]以外)
    /// @param parameter_name パラメータ名(enum)
    /// @param parameter 読み出したパラメタを保存する変数
    /// @return True:成功   False:失敗
    template <typename T>
    bool read(const E_ParameterCategories parameter_name, T& parameter){
        if (DEBUG){Serial.print("Method-read_template: ");}
        if (parameter_name != E_ParameterCategories::SERIAL_NUMBER){
            const uint16_t idx = static_cast<uint16_t>(parameter_name);
            uint8_t* ptr = (uint8_t *) &parameter;
            for (int i=0; i < sizeof(parameter); i++){
                *(ptr + i) = Adafruit_FRAM_I2C::read8(idx + i);
            }
            return true;
        };
        return false;
    };

    /// @brief parameterをFRAMに書き込む(任意の型 char[]以外)
    /// @param parameter_name パラメータ名(enum)
    /// @param parameter パラメタ
    /// @return True:成功   False:失敗
    template <typename T>
    bool store(const E_ParameterCategories parameter_name, const T& parameter){
        if (DEBUG){Serial.print("Method-store_template: ");}
        if (parameter_name != E_ParameterCategories::SERIAL_NUMBER){
            const uint16_t idx = static_cast<uint16_t>(parameter_name);
            const uint8_t* ptr = (const uint8_t *) &parameter;
            for (int i=0; i < sizeof(parameter); i++){
                Adafruit_FRAM_I2C::write8(idx + i, *(ptr + i));
            }
            return true;
        };
        return false;
    };

    bool read(const E_ParameterCategories parameter_name, char* parameter);
    bool store(const E_ParameterCategories parameter_name, const char* parameter, size_t len);

    private:
    // consts

    // debug flag
    static constexpr bool DEBUG = false;

    // instances

    // vars
    
    // methods 
};

#endif //_PARAMETERSTORAGE_H_