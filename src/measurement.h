/**************************************************************************/
/*!
 * @file 
 * @brief 
 * @author 
 * @date 20230908
 * $Version:    0.0$
 * @par 
 *    
 * 
 */
/**************************************************************************/

#ifndef _MEASUREMENT_H_
#define _MEASUREMENT_H_

#include <Arduino.h>

// デバイスのドライバ
#include <Adafruit_ADS1015.h>   // ADC 16bit diff - 2ch
#include <Adafruit_MCP23008.h>  // PIO 8bit
#include <Adafruit_MCP4725.h>   // DAC  12bit 
#include "DAC80501.h"           // DAC 16bit for Analog Mon Out

class Measurement {

    public:
    // consts

    /*!
    * @brief 計測ユニットへのコマンド一覧
    */
    enum class ECommand : uint8_t{
        IDLE = 0,   // 何もしない、現状のまま
        START,      //  測定開始
        STOP        //  測定停止
    };

    /*!
    * @brief 動作モード一覧
    */
    enum class EModes{
        MANUAL = 0,
        TIMER,
        CONTINUOUS
    };
    
    // @brief 計測のための設定値、校正値を保存する構造体 //24byte
    struct MesasUintParameters{
        //  センサ長 [inch]
        uint8_t sensor_length;
        //  タイマ設定  [s]
        uint16_t timer_period;
        //  スケーリング
        //      100%表示をセンサ長の何%に設定するか [0.1%]
        uint16_t scale_100 = 1000;
        //      0%表示をセンサ長の何%に設定するか [0.1%]
        uint16_t scale_0 = 0;
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

 

    // ユニットの設定
    void setCommand(Measurement::ECommand command);
    void setMode(Measurement::EModes mode);
    EModes getMode(void);

    // 測定関連
    bool isReady(void);
    bool shouldMeasure(void);
    void executeMeasurement(void);
    bool isSensorError(void); 
    bool isResultReady(void); 
    uint16_t getResult(void); 

    //  statemachineへのフィードバック 
    bool haveFinishedMeasurement(void); //正常測定完了信号      statemachine用    モーメンタリ
    bool haveFailedMesasurement(void);  //測定開始エラー信号    statemachine用    モーメンタリ

    //  I2Cバス制御
    bool shouldVacateI2Cbus(void);

    //  モニタ出力制御
    void setVmon(const uint16_t& vout);
    void setVmonFailed(void);

    private:
    // consts

    // debug flag
    constexpr static bool DEBUG = true;

    // instances
    // デバイスのインスタンスへのポインタ 
    //  電流設定用DAコンバータ
    Adafruit_MCP4725*   current_adj_dac = nullptr;
    // //  アナログモニタ出力用DAコンバータ
    DAC80501*           v_mon_dac = nullptr;
    // //  電流源制御用    GPIO
    Adafruit_MCP23008*  pio = nullptr;
    // //  電圧・電流読み取り用ADコンバータ
    Adafruit_ADS1115*   meas_adc = nullptr;

    // vars

    //  センサ抵抗値[ohm]
    float sensor_resistance = 0.0;
    //  熱伝導待ち時間 [ms]
    uint16_t sensor_heat_propagation_time;
    //  液面計測結果
    uint16_t measured_level = 0;

    //  センサエラーフラグ
    bool sensor_error = false;

    // リソースが命令実行中
    bool busy_now = false;

    //  外部への測定指示フラグ
    bool should_measure = false;

    // 一回計測終了のフラグ
    bool finished_single_meas = false;

    // 測定開始失敗のフラグ
    bool failed_meas = false;

    // 測定終了（測定結果確定）のフラグ
    bool result_ready = false;

    // I2Cバス占有したいフラグ
    bool occupy_the_bus = false;

    // 連続計測動作
    uint16_t cont_meas_inteval_counter = 0;

     // 一回計測動作
    uint16_t single_meas_counter = 0;
    uint16_t single_meas_interval = 0;
    uint16_t single_meas_period = 0;
    bool single_last_meas = false;

    // 計測用ADCゲイン係数 mirco volt/LSB
    float adc_gain_coeff = 0.0;

    //  現在の動作モードを保持
    EModes present_mode = EModes::TIMER;

    // methods 

    //  電流源制御
    bool currentOn(void);
    void currentOff(void);
    void setCurrent(const uint16_t& current = 750);
    bool getCurrentSourceStatus(void);

    // 計測制御
    void terminateMeasurement(void);

    //  電圧・電流値の読み取り
    int32_t  read_raw_voltage(const uint8_t channel);
    uint32_t read_voltage(void);
    uint32_t read_current(void);
    uint16_t read_level(void);
    void level_scaling(int16_t& level, const uint16_t& hiside_scle = 1000, const uint16_t& lowside_scale = 0);

};

#endif //_MEASUREMENT_H_