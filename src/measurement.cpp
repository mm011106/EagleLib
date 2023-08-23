/*！

*/

#include "measurement.h"

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

// Methodの実体

/// @brief 内部パラメタの設定、デバイスドライバインスタンスの作成・初期化
void Measurement::init(void){
    // センサ長から内部パラメタを計算
    sensor_resistance = SENSOR_UNIT_IMP * (float)p_parameter->sensor_length;
    sensor_heat_propagation_time = p_parameter->sensor_length * (uint16_t)(1/HEAT_PROPERGATION_VEROCITY * 1000.0 * 1.2); // [ms]
    single_meas_period = sensor_heat_propagation_time/10;//[CLK count, clk=10ms cycle]
    single_meas_interval = single_meas_period/3;//[CLK count]

    Serial.println(sensor_heat_propagation_time);
    Serial.print("Sensor Length:"); Serial.println(p_parameter->sensor_length);
    Serial.print("Delay Time:"); Serial.println(sensor_heat_propagation_time);
    Serial.print("Sensor R:"); Serial.println(sensor_resistance);
    // デバイスの校正値を設定
    // デバイスの校正値はFramから読み込まれてp_parameterに入っているのでそこを直接読む
    
    // deviceドライバのインスタンス作成、初期化
    // たとえば
    // if(pio){
    //     delete pio;
    // }
    // pio = new Adafruit_MCP23008;
    // if (pio->begin(I2C_ADDR::PIO, &Wire)) { 
    //     //  set IO port 
    //     pio->pinMode(PIO_PORT::CURRENT_ERRFLAG, INPUT);
    //     pio->pullUp(PIO_PORT::CURRENT_ERRFLAG, HIGH);  // turn on a 100K pullup internally

    //     pio->pinMode(PIO_PORT::CURRENT_ENABLE, OUTPUT);
    //     pio->digitalWrite(PIO_PORT::CURRENT_ENABLE, CURRENT_OFF);
    // } else {
    //     Serial.println("error on PIO.  ");
    //     f_init_succeed = false;
    // }
    
    // 確認として、インスタンスのアドレスとサイズを印字
    // Serial.print("DA-current:"); Serial.print((uint32_t)current_adj_dac,HEX); Serial.print("/");Serial.println(sizeof(*current_adj_dac));
    // Serial.print("DA-Vmon:"); Serial.print((uint32_t)v_mon_dac,HEX); Serial.print("/");Serial.println(sizeof(*v_mon_dac));
    // Serial.print("PIO:"); Serial.print((uint32_t)pio,HEX); Serial.print("/");Serial.println(sizeof(*pio));
    // Serial.print("ADC:"); Serial.print((uint32_t)adconverter,HEX); Serial.print("/");Serial.println(sizeof(*adconverter));
    // Serial.print("eh900:"); Serial.print((uint32_t)LevelMeter,HEX); Serial.print("/");Serial.println(sizeof(*LevelMeter));


}  

/// @brief CLKに同期した処理を行います 
/// @note 連続計測の計測周期の管理を行っています  
void Measurement::clk_in(void){
    bool current_busy_status = busy_now;
    busy_now = true;
 
    //  CLKに同期した処理を記載
    // 連続計測の処理
    if (cont_measurement){
        //  1秒に一回計測
        if (cont_meas_inteval_counter++ > CONT_MEAS_INTERVAL){
            cont_meas_inteval_counter=0;
            if (getCurrentSourceStatus()){
                sensor_error = false;
                read_level();
            } else {
                sensor_error = true;
            }
        };
        if (sensor_error){
            Serial.println("CONT Treminate by error.");
            //センサエラー（測定中にエラー発生）なら計測を終了して帰る
            setCommand(Measurement::ECommand::CONTEND);
            return;
        };
    };

    // 1回計測の処理
    if (single_measurement){
        // 熱伝搬時間の1/3ごとに計測
        if (single_meas_counter++ < single_meas_period){
            if ( (single_meas_counter % single_meas_interval) == 0){
                if (getCurrentSourceStatus()){
                    sensor_error = false;
                    read_level();
                } else {
                    sensor_error = true;
                }
            }
        }else{
            if (getCurrentSourceStatus()){
                sensor_error = false;
                read_level();
            } else {
                sensor_error = true;
            }
            single_measurement = false;
        };
        if (sensor_error){
            Serial.println("SINGLE Treminate by error.");
            //センサエラー（測定中にエラー発生）なら計測を終了して帰る
            currentOff();
            single_measurement = false;

            return;
        };
    };

    // // リソースの占有解放、busy信号の作成
    // if(acquire_resorce){
    //     busy_now = false; // 占有処理完了としてbusyをネゲート
    //     acquire_resorce = false;
    // }

    // if(free_resorce){
    //     if(--keep_busy==0){
    //         resource_available=true;
    //         busy_now = true;//  解放されたことで動作できなくなるので、busyをアサート
    //         free_resorce=false;
    //         keep_busy=DEFAULT_KEEP_BUSY;
    //         return;
    //     }
    // }


    // 同期信号処理のクロージング処理
    busy_now = current_busy_status;

    return;
};

/// @brief 計測クラスに計測の命令を与えます
/// @param ECommand::command  命令 
void Measurement::setCommand(Measurement::ECommand command){
    switch (command)
    {
    case Measurement::ECommand::SINGLE :
        if (!busy_now){
            present_mode = EModes::MANUAL;
            busy_now = true;
            if (measSingle()){
                Serial.println("Single MEAS complete.");
                sensor_error = false;
            } else {
                Serial.println("Single MEAS ERROR.");
                sensor_error = true;
            }
            present_mode = EModes::TIMER;
            busy_now = false;
        } else {
            Serial.println("Fail: measure command while busy.");
        }
        break;

    case Measurement::ECommand::CONTSTART :
        if(!busy_now){
            present_mode = EModes::CONTINUOUS;
            busy_now = true;
            if (contSTART()){
                Serial.println("CONT Start.");
                sensor_error = false;
            } else {
                Serial.println("CONT Meas ERROR. terminate");
                sensor_error = true;
            }
        } else {
            Serial.println("Fail: measure command while busy.");
        }
        break;
    
    case Measurement::ECommand::CONTEND :
        if(busy_now){
            if (contEND()){
                Serial.println("CONT END.");
                sensor_error = false;
                present_mode = EModes::TIMER;
                busy_now = false;
            } else {
                Serial.println("CONT Meas END ERROR.");
                sensor_error = true;
            }
        } else {
            Serial.println("Fail: CONT END command while ready.");
        }
        break;

    case Measurement::ECommand::IDLE :
        Serial.print("busy:");Serial.print(busy_now);Serial.print(" - error:");Serial.print(sensor_error);
        Serial.print(" - cont:");Serial.println(cont_measurement);
        Serial.println("Measurement status did not change.");
        break;

    default:
        Serial.println("ERROR: NO COMMAND");
        break;
    }
}

/*!
 * @brief 電流源をonにする
 */
bool Measurement::currentOn(void){
    Serial.println("CurrentSoruce ON");
    return true;
    // return false;
}

/*!
 * @brief 電流源をoffにする
 */
void Measurement::currentOff(void){
    Serial.println("CurrentSoruce OFF");
    return ;
}

/*!
 * @brief 電流源を設定する
 * @param current 設定電流値[0.1mA]
 */
void Measurement::setCurrent(uint16_t current){
    Serial.print("CurrentSoruce set");Serial.println(current);
    return;
}

/*!
 * @brief 電流源のステータスを返す
 * @returns True: 電流Onの設定で正常に電流を供給している, False:電流がoff もしくは 負荷異常
 */
bool Measurement::getCurrentSourceStatus(void){
    Serial.print("C-C ");
    // return false;
    return true;
}

/// @brief 一回計測
/// @param void 
/// @return bool true:エラーなしで計測完了  false:エラー発生
/// @note エラー時は電流をoffにしてsensor_errorをtrueにして帰る 
bool Measurement::measSingle(void){
    Serial.print("Meas:: single meas.. ");
    uint16_t reading = 0;
    if (currentOn()){
        for (size_t i = 0; i < 3; i++){
            delay(sensor_heat_propagation_time/3);
            if (getCurrentSourceStatus()){
                sensor_error = false;
                reading = read_level();
            } else {
                Serial.println("SNGL terminate by error.");
                sensor_error = true;
                currentOff();
                break;
            }
        }
        if (getCurrentSourceStatus()){
            sensor_error = false;
            reading = read_level();
        }
        currentOff();
        Serial.println("::Meas END.");
    } else {
        currentOff();
        return false;
    };

    return true;
}

/// @brief 連続計測を開始するための準備（CLK非同期処理）
/// @param void 
/// @return bool true:正常に開始できた false:開始できなかった 
bool Measurement::contSTART(void){
    bool status = currentOn();
    if (status){
        cont_measurement = true;
    }
    return status;
}

/// @brief 連続計測を終了させるための準備（CLK非同期処理）
/// @param void 
/// @return 常にtrue:正常終了 
bool Measurement::contEND(void){
    cont_measurement = false;
    currentOff();
    return true;
}

/// @brief 液面計速を実行
/// @param void 
/// @return uint16_t 液面 [0.1%] 
uint16_t Measurement::read_level(void){
    Serial.print("RL ");
    return 1234;
}

/// @brief 電圧モニタ出力を設定する 
/// @param vout 出力電圧[0.1V] 
void Measurement::setVmon(uint16_t vout){
    Serial.print("Vout: set");Serial.println(vout);
    return;
}

/// @brief  電圧モニタ出力にエラーを提示する（0V) 
/// @param  void
void Measurement::setVmonFailed(void){
    Serial.println("Vout: Error indicate.");    
}