/**************************************************************************/
/*!
 * @file 
 * @brief 
 * @author 
 * @date 
 * $Version:    0.0$
 * @par 
 *    
 * 
 */
/**************************************************************************/

#ifndef _MEASUREMENT_H_
#define _MEASUREMENT_H_

// デバイスのドライバ
#include <Adafruit_ADS1015.h>   // ADC 16bit diff - 2ch
#include <Adafruit_MCP23008.h>  // PIO 8bit
#include <Adafruit_MCP4725.h>   // DAC  12bit 
// #include "DAC80501.h"           // DAC 16bit for Analog Mon Out

class Measurement {

    public:
    // consts

    /*!
    * @brief 計測ユニットへのコマンド一覧
    */
    enum class ECommand : uint8_t{
        IDLE = 0,
        SINGLE,
        CONTSTART,
        CONTEND,
        TERMINATE
    };

    /*!
    * @brief 動作モード一覧
    */
    enum class EModes{MANUAL = 0, TIMER, CONTINUOUS};
    
    // @brief 計測のための設定値、校正値を保存する構造体 //20byte
    struct MesasUintParameters{
        //  センサ長 [inch]
        uint8_t sensor_length;
        //  タイマ設定  [s]
        uint16_t timer_period;

        //  計測ユニットの校正値
        //      ADコンバータのエラー補正系数    電圧計測
        float_t adc_err_comp_diff_0_1;
        //      ADコンバータのエラー補正系数    電流計測
        float_t adc_err_comp_diff_2_3;
        //      ADコンバータのオフセット補正    電圧計測
        int16_t adc_OFS_comp_diff_0_1;
        //      ADコンバータのオフセット補正    電流計測
        int16_t adc_OFS_comp_diff_2_3;
        //      電流源設定値    [0.1mA] 
        uint16_t current_set_default;
        //      アナログモニタ出力DAのオフセット（0.1V出力時の誤差）  [LSB] 
        uint16_t vmon_da_offset;
    };


    // instances

    // vars
    const MesasUintParameters* p_parameter;
    
    // methods 
    /*!
    * @brief constructor  計測パラメタへのポインタを内容immutableとして受け取ります 
    */
    Measurement(const MesasUintParameters* const ptr){
        p_parameter = ptr;
    };

    /*!
    * @brief deconstructor
    *  
    */
    ~Measurement(){
    };


    void init(void);
    void clk_in(void);

    /// @brief 計測クラスが動作中かどうか
    /// @return True:測定可能   False:現在作業中
    bool isReady(void){
        return !busy_now;
    };

    void setCommand(Measurement::ECommand command);

    // 実際の測定を実行
    void executeMeasurement(void);
    bool shouldMeasure(void){
        return should_measure;
    }

    EModes getMode(void){
        return present_mode;
    };

    //  電流源制御
    bool currentOn(void);
    void currentOff(void);
    void setCurrent(uint16_t current = 750);
    bool getCurrentSourceStatus(void);

    //  モニタ出力制御
    void setVmon(uint16_t vout);
    void setVmonFailed(void);

    private:
    // consts

    // instances
    // デバイスのインスタンスへのポインタ 
    //  電流設定用DAコンバータ
    // Adafruit_MCP4725*   current_adj_dac = nullptr;
    // //  アナログモニタ出力用DAコンバータ
    // DAC80501*           v_mon_dac = nullptr;
    // //  電流源制御用    GPIO
    // Adafruit_MCP23008*  pio = nullptr;
    // //  電圧・電流読み取り用ADコンバータ
    // Adafruit_ADS1115*   adconverter = nullptr;

    // vars

    //  センサ抵抗値[ohm]
    float sensor_resistance = 0.0;
    //  熱伝導待ち時間 [ms]
    uint16_t sensor_heat_propagation_time;

    //  センサエラーフラグ
    bool sensor_error = false;

    // リソースが命令実行中
    bool busy_now = false;

    //  外部への測定指示フラグ
    bool should_measure = false; 

    // 連続計測動作
    uint16_t cont_meas_inteval_counter = 0;

     // 一回計測動作
    uint16_t single_meas_counter = 0;
    uint16_t single_meas_interval = 0;
    uint16_t single_meas_period = 0;
    bool single_last_meas = false;

    //  現在の動作モードを保持
    EModes present_mode = EModes::TIMER;

    // methods 
    void terminateMeasurement(void);

    //  電圧・電流値の読み取り
    uint32_t read_voltage(void);
    uint32_t read_current(void);
    uint16_t read_level(void);

};

#endif //_MEASUREMENT_H_