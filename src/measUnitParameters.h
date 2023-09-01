#ifndef _MEASUNITPARAMETERS_H_
#define _MEASUNITPARAMETERS_H_

// consts
//  I2C adress 計測用のIC達
    namespace I2C_ADDR{
    constexpr uint16_t ADC            = 0x48;   // 電圧・電流計測
    constexpr uint16_t CURRENT_ADJ    = 0x60;   // 電流源調整用DA
    constexpr uint16_t V_MON          = 0x49;   // 電圧出力用DAコンバータ
    constexpr uint16_t PIO            = 0x20;   // GPIO IC  電流on/off  電流源エラー読み込み
    };

// ADの読み値から電圧値を計算するための系数 [/ micro Volts/LSB]
// 3.3V電源、差動計測（バイポーラ出力）を想定
    namespace ADC_READOUT_VOLTAGE_COEFF{
    constexpr float GAIN_TWOTHIRDS    = 187.506;  //  FS 6.144V * 1E6/32767
    constexpr float GAIN_ONE          = 125.004;  //  FS 4.096V * 1E6/32767
    constexpr float GAIN_TWO          = 62.5019;  //  FS 2.048V * 1E6/32767
    constexpr float GAIN_FOUR         = 31.2510;  //  FS 1.024V * 1E6/32767
    constexpr float GAIN_EIGHT        = 15.6255;  //  FS 0.512V * 1E6/32767
    constexpr float GAIN_SIXTEEN      = 7.81274;  //  FS 0.256V * 1E6/32767
    };

//  PIO関連 定数
    namespace PIO_PORT{
    //  PIOのポート番号の設定と論理レベル設定
    constexpr uint16_t CURRENT_ENABLE    = 4 ;  // ON = LOW,  OFF = HIGH
    constexpr uint16_t CURRENT_ERRFLAG   = 0 ;  // LOW = FAULT(short or open), HIGH = NOMAL
    };

    // 電流源コントロールロジック定義
    constexpr uint16_t CURRENT_OFF  = HIGH ;    // Current off when GPIO=HIGH
    constexpr uint16_t CURRENT_ON   = LOW ;     // Current on when GPIO=LOW

//  計測に使う定数
    //  センサの単位長あたりのインピーダンス[ohm/inch]
    constexpr float SENSOR_UNIT_IMP = 11.6;

    //  センサの熱伝導速度  測定待ち時間の計算に必要  inch/s
    constexpr float HEAT_PROPERGATION_VEROCITY = 7.9; 

    // 電流計測時の電流電圧変換係数(R=5kohm, Coeff_Isenosr=0.004(0.2/50ohm)の時) [/ V/A]
    constexpr float CURRENT_MEASURE_COEFF = 20.000;

    //  電圧計測のアッテネータ系数  1/10 x 2/5 の逆数  実際の抵抗値での計算
    constexpr float ATTENUATOR_COEFF = 24.6642;

    // AD変換時の平均化回数 １回測るのに10msかかるので注意  10回で100ms
    constexpr uint16_t ADC_AVERAGE_DEFAULT = 10;

    //  電流源調整用DAC MCP4725 1Vあたりの電流[0.1mA/V] 56==5.6mA/V
    constexpr uint16_t  CURRENT_SORCE_VI_COEFF  = 56; 

    //  電流源調整用DAC MCP4275の1Vあたりのカウント (3.3V電源にて） COUNT/V
    constexpr uint16_t DAC_COUNT_PER_VOLT = 1241;

    //  Vmon用  DAC80501 1Vあたりのカウント(2.5VFS時）  COUNT/V
    constexpr uint16_t VMON_COUNT_PER_VOLT = 26214;

    // 計測用定数
    //  連続計測時の計測周期
    constexpr uint16_t CONT_MEAS_INTERVAL = 100; // [x10ms]


#endif //_MEASUNITPARAMETERS_H_