/*！

*/

#include "measurement.h"
#include "measUnitParameters.h"

// Methodの実体

/// @brief 内部パラメタの設定、デバイスドライバインスタンスの作成・初期化
void Measurement::init(void){
    // センサ長から内部パラメタを計算
    sensor_resistance = SENSOR_UNIT_IMP * (float)p_parameter->sensor_length;
    sensor_heat_propagation_time = p_parameter->sensor_length * (uint16_t)(1/HEAT_PROPERGATION_VEROCITY * 1000.0 * 1.2); // [ms]
    single_meas_period = sensor_heat_propagation_time/10;//[CLK count, clk=10ms cycle]
    // 一回計測時の伝搬時間内の計測周期を決める 3回計測することを基本にする
    // 6インチぐらいの短いセンサだと伝搬時間内に３回測定できないので、最短で0.5秒とし、それをlimitとする
    if (single_meas_period < 150){ // 1.5秒より短い伝搬時間の場合
        single_meas_interval = 50; // 0.5s周期
    } else{
        single_meas_interval = single_meas_period/3;//[CLK count]   
    }

    if(DEBUG){
        Serial.print("Sensor Length[inch]:"); Serial.println(p_parameter->sensor_length);
        Serial.print("Delay Time:"); Serial.println(sensor_heat_propagation_time);
        Serial.print("Sensor R:"); Serial.println(sensor_resistance);
        Serial.print("TimerPeriod: "); Serial.println(p_parameter->timer_period);
        Serial.print("setCurrent: "); Serial.println(p_parameter->current_set_default);
        Serial.print("VmonDA_offset: "); Serial.println(p_parameter->vmon_da_offset);
    }
    // デバイスの校正値を設定
    // デバイスの校正値はFramから読み込まれてp_parameterに入っているのでそこを直接読む
    
    // deviceドライバのインスタンス作成、初期化

    bool f_init_succeed = true;

    // 電流源設定用DAC  初期化
    if(current_adj_dac){delete current_adj_dac;}
    current_adj_dac = new Adafruit_MCP4725;
    if (current_adj_dac->begin(I2C_ADDR::CURRENT_ADJ, &Wire)) { 
        // 電流値設定
        setCurrent(p_parameter->current_set_default);
    } else {
        if(DEBUG){Serial.println("error on Current Source DAC.  ");}
        f_init_succeed = false;
    }

    // アナログモニタ用DAC  初期化
    if(v_mon_dac){delete v_mon_dac;}
    v_mon_dac = new DAC80501;
    if (v_mon_dac->begin(I2C_ADDR::V_MON, &Wire)) { 
         if (v_mon_dac->init()) { 
            // アナログモニタ出力   リセット 
            setVmon(0);
        } else {
            if(DEBUG){Serial.println("error on Analog Monitor DAC.  ");}
            f_init_succeed = false;
        }
    } else {
        if(DEBUG){Serial.println("error on Analog Monitor DAC.  ");}
        f_init_succeed = false;
    }

    // PIO  初期化
    if(pio){delete pio;}
    pio = new Adafruit_MCP23008;
    if (pio->begin(I2C_ADDR::PIO, &Wire)) { 
        //  set IO port 
        pio->pinMode(PIO_PORT::CURRENT_ERRFLAG, INPUT);
        pio->pullUp(PIO_PORT::CURRENT_ERRFLAG, HIGH);  // turn on a 100K pullup internally

        pio->pinMode(PIO_PORT::CURRENT_ENABLE, OUTPUT);
        pio->digitalWrite(PIO_PORT::CURRENT_ENABLE, CURRENT_OFF);
    } else {
        if(DEBUG){Serial.println("error on PIO.  ");}
        f_init_succeed = false;
    }

    //  計測用ADコンバータ設定    PGA=x2   2.048V FS
    if(meas_adc){delete meas_adc;}
    meas_adc = new Adafruit_ADS1115(I2C_ADDR::ADC);
    meas_adc->begin();
    meas_adc->setGain(adsGain_t::GAIN_TWO); 

    // 確認として、インスタンスのアドレスとサイズを印字
    if(DEBUG){
        Serial.print("DA-current:"); Serial.print((uint32_t)current_adj_dac,HEX); Serial.print("/");Serial.println(sizeof(*current_adj_dac));
        Serial.print("DA-Vmon:"); Serial.print((uint32_t)v_mon_dac,HEX); Serial.print("/");Serial.println(sizeof(*v_mon_dac));
        Serial.print("PIO:"); Serial.print((uint32_t)pio,HEX); Serial.print("/");Serial.println(sizeof(*pio));
        Serial.print("ADC:"); Serial.print((uint32_t)meas_adc,HEX); Serial.print("/");Serial.println(sizeof(*meas_adc));
    }


}  

/// @brief CLKに同期した処理を行います 
/// @note 連続計測の計測周期の管理を行っています  
void Measurement::clk_in(void){
    bool current_busy_status = busy_now;
    busy_now = true;
 
    //  CLKに同期した処理を記載
    // 連続計測の処理
    if (present_mode == EModes::CONTINUOUS){
        //  1秒に一回計測
        if (cont_meas_inteval_counter++ > CONT_MEAS_INTERVAL){
            cont_meas_inteval_counter=0;
            should_measure = true;
        };

    };

    // 1回計測の処理     
    if (present_mode == EModes::MANUAL){
        // 熱伝搬時間の1/3ごとに計測
        //      伝搬時間中に３回計測して、２CLK余分に時間待ってから最終計測(else節）を実行
        //      should_measureフラグがCLK時間で連続して立たないように配慮
        if (single_meas_counter++ <= (single_meas_period + 2)){
            if ( (single_meas_counter % single_meas_interval) == 0){
                single_last_meas = false;
                should_measure = true;
                if(DEBUG){Serial.print("preMeas ");}
            }
        }else{
            // 最終の計測：ここでの計測が測定値として保持される

            // 上の測定要求と以下の測定要求の間は最短で30ms(2CLK余計に回って次のCLKでここにくる)しかないので
            // 上の測定を実行している可能性がある。
            // single_last_measは一回tureになったら、それ以降再設定しないように処理
            // 計測処理に時間が必要なので、複数回呼ばれて計測を複数回実行してしまうこを阻止する
            if (!single_last_meas){
                single_last_meas = true;
                should_measure = true;
                // exec_single_measurement = false;
                if(DEBUG){Serial.print("lastMeas ");}
            }
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

    switch (command){
        case Measurement::ECommand::START :
            if (!busy_now){
                busy_now = true;
                if (currentOn()){
                    sensor_error = false;
                    if (present_mode == EModes::MANUAL){//一回計測の準備
                        if(DEBUG){Serial.print("SINGLE Start. ");Serial.println(micros());}
                        single_meas_counter = 0;
                    }
                    if (present_mode == EModes::CONTINUOUS){// 連続計測の準備
                        if(DEBUG){Serial.println("CONT Start.");}
                    }
                }else{
                    if(DEBUG){Serial.println("MeasCommand ERROR. terminate");}
                    sensor_error = true;
                    terminateMeasurement();
                }
            } else {
                if(DEBUG){Serial.println("Fail: measure command while busy.");}
            }
            break;

        case Measurement::ECommand::STOP :
            // if(busy_now){
            //     terminateMeasurement();
            // } else {
            //     if(DEBUG){Serial.println("OK: anyway, STOP measurement.");}
            // }
            if(DEBUG){Serial.println("Accept STOP command.");}
            terminateMeasurement();
            break;


        case Measurement::ECommand::IDLE :
            if(DEBUG){Serial.print("busy:");Serial.print(busy_now);Serial.print(" - error:");Serial.print(sensor_error);}
            // Serial.print(" - cont:");Serial.println(exec_cont_measurement);
            if(DEBUG){Serial.println("Measurement status did not change.");}
            break;

        default:
            if(DEBUG){Serial.println("ERROR: NO COMMAND");}
            break;
    }
}

/// @brief 計測モードをセットする
void Measurement::setMode(Measurement::EModes mode){
    present_mode = mode;
    return;
}

/// @brief 現在の計測モードを返す
/// @return EModes
Measurement::EModes Measurement::getMode(void){
    return present_mode;
};

/// @brief 計測クラスが動作中かどうか
/// @return True:測定可能   False:現在作業中
bool Measurement::isReady(void){
    return !busy_now;
};

/// @brief 測定要求
/// @return True:測定してください   False:何もしなくていいです
/// @note 測定開始のタイミングはmain()で制御します。このフラグを読んで計測を開始してください。
bool Measurement::shouldMeasure(void){
    return should_measure;
};

/*!
 * @brief 実際の計測動作を行う  エラー時は測定を中断する    一回計測の終了判断を行い終了させる
 * @note 実行には100ms程度かかる
 */
void Measurement::executeMeasurement(void){
    // 測定指示フラグをここでクリア
    // これ以降のタイミングでフラグが立てば、それは保持される
    // フラグが立っている時間は最長で、CLKisrの処理時間、メインルーチンへの復帰時間、メイン内部処理一巡
    // となり、10ms以下を期待できる。
    should_measure = false;
    bool the_last_meas = single_last_meas;

    // for debug
    if(DEBUG){Serial.print("execMeas::start "); Serial.print(micros());Serial.print(" ");}

    // uint16_t result = 0;
    if (getCurrentSourceStatus()){
        sensor_error = false;
        measured_level = read_level();
        result_ready = true;
    } else {
        sensor_error = true;
    }

    // Sensorエラー処理（異常終了）
    if (sensor_error){
        if(DEBUG){Serial.print("-sensorError  - ");}
        //センサエラー（測定中にエラー発生）なら計測を終了して帰る
        if(DEBUG){Serial.println("Measurement Treminate by error.");}
        terminateMeasurement();
        return;
    }

    // 一回計測の最終計測の場合は一回計測のクロージング処理
    if (the_last_meas){
        if(DEBUG){Serial.print("-single:last- ");}
        single_last_meas = false;
        single_meas_counter = 0;
        should_measure = false;//最終計測なので、計測中に入った測定要求は無視する
        finished_single_meas = true; // 一回計測完了のフラグ
        terminateMeasurement();
    }

    // 測定結果を出力する処理
    // 電圧モニタへの出力
    // !!!  このクラスとVmon出力が強く結びついているが、それでいいか？？？
    //      外部で出力するようにしなくてもいいか？？？
    setVmon(measured_level);

    if(DEBUG){Serial.println("execMeas::End ");}
}

/// @brief I2Cバスの明け渡し要求
/// @return true:明け渡しが必要 false:不要
bool Measurement::shouldVacateI2Cbus(void){
    return occupy_the_bus;
}

/// @brief 1回計測が終了したことを読み出す
/// @return True:終了 False:測定中
/// @note 一度読み出すとfalseにリセットされます
bool Measurement::haveFinishedMeasurement(void){
    bool temp = finished_single_meas;
    finished_single_meas = false;
    return temp;
}

/// @brief 測定が異常終了したことを読み出す
/// @return True:異常終了 False:正常
/// @note 一度読み出すとfalseにリセットされます
bool Measurement::haveFailedMesasurement(void){
    bool temp = failed_meas;
    failed_meas = false;
    return temp;
}

/// @brief センサエラーの状態を読み出す
/// @return True:エラー False:正常
/// @note LCD表示への信号
bool Measurement::isSensorError(void){
    return sensor_error;
}

/// @brief 新しい測定結果があるかどうか
/// @return True:あり   False:前回の結果のまま
/// @note 一度読み出すとfalseにリセットされます
bool Measurement::isResultReady(void){
    bool temp = result_ready;
    result_ready = false;
    return temp;
}

/// @brief 測定結果を読み出す
/// @return 液面値 [0.1%]
/// @note 読み出しは常時可能ですが、最新かどうかはisResultReady()で確認してください。
uint16_t Measurement::getResult(void){
    return measured_level;
}

/*!
 * @brief 電流源をonにする
 */
bool Measurement::currentOn(void){
    // if(DEBUG){Serial.println("CurrentSoruce ON");}


    Serial.print("currentCtrl:ON -- "); 
    pio->digitalWrite(PIO_PORT::CURRENT_ENABLE, CURRENT_ON);
    delay(10); // エラー判定が可能になるまで10ms待つ
    
    if (pio->digitalRead(PIO_PORT::CURRENT_ERRFLAG) == LOW){
        // f_sensor_error = true;
        pio->digitalWrite(PIO_PORT::CURRENT_ENABLE, CURRENT_OFF);
        Serial.print(" FAIL.  ");
    } else {
        // f_sensor_error = false;
        delay(100); // issue1: 電流のステイブルを待つ
        Serial.print(" OK.  ");
    }
    Serial.println("Fin. --");

    return true;


    // FOR TEST
    // return false;
    // long rand = random(100);
    // if (rand > 30){
    //     Serial.println("-Normal");
    //     return true;
    // }else {
    //     Serial.println("-Fail");
    //     return false;
    // }
}

/*!
 * @brief 電流源をoffにする
 */
void Measurement::currentOff(void){
    // if(DEBUG){Serial.println("CurrentSoruce OFF");}
    Serial.print("currentCtrl:OFF  -- ");
    pio->digitalWrite(PIO_PORT::CURRENT_ENABLE, CURRENT_OFF);      
    Serial.println(" Fin. --");
    return ;
}

/*!
 * @brief 電流源を設定する
 * @param current 設定電流値[0.1mA]
 */
void Measurement::setCurrent(uint16_t current){
    if(DEBUG){Serial.print("CurrentSoruce set ");Serial.println(current);}
    if ( 670 < current && current < 830){
        uint16_t value = (( current - 666 ) * DAC_COUNT_PER_VOLT) / CURRENT_SORCE_VI_COEFF;
        Serial.print(" value:"); Serial.print(value);
        // current -> vref converting function
        current_adj_dac->setVoltage(value, false);
        Serial.println(" - DAC changed. " ); 
      }
    return;
}

/*!
 * @brief 電流源のステータスを返す
 * @returns True: 電流Onの設定で正常に電流を供給している, False:電流がoff もしくは 負荷異常
 */
bool Measurement::getCurrentSourceStatus(void){
    if(DEBUG){Serial.print("C-C ");}

    return (   (pio->digitalRead(PIO_PORT::CURRENT_ENABLE ) == CURRENT_ON)  \
            && (pio->digitalRead(PIO_PORT::CURRENT_ERRFLAG) == HIGH)        \
    );

    // FOR TESST
    // long rand = random(100);
    // if (rand > 20){
    //     return true;
    // }else {
    //     return false;
    // }
}

/// @brief 電圧モニタ出力を設定する 
/// @param vout 出力電圧[0.1V] 
void Measurement::setVmon(const uint16_t vout){
    if(DEBUG){Serial.print("Vout: set ");Serial.println(vout);}
    uint16_t da_value=0;

    //  100.0%以下の値ならそのまま設定、100.0%以上なら100.0%として設定
    if (vout <= 1000) {
        // da_value = ( VMON_COUNT_PER_VOLT * (value + 100) ) / 1000;
        da_value = (( VMON_COUNT_PER_VOLT * vout ) / 1000) + (uint16_t)((VMON_COUNT_PER_VOLT / 10) - p_parameter->vmon_da_offset );

    //     100.0% = 1.1V, 0%=0.1V 
    } else {
        da_value = (( VMON_COUNT_PER_VOLT * 1000 ) / 1000) + (uint16_t)((VMON_COUNT_PER_VOLT / 10) - p_parameter->vmon_da_offset );
    }
    v_mon_dac->setVoltage(da_value);

    return;
}

/// @brief  電圧モニタ出力にエラーを提示する（0V) 
/// @param  void
void Measurement::setVmonFailed(void){
    if(DEBUG){Serial.println("Vout: Error indicate.");}
    v_mon_dac->setVoltage((uint16_t) 0);
    return;
}

// 
// Private methods
// 

//
// @brief 測定を終了する
//
void Measurement::terminateMeasurement(void){
    should_measure = false;
    currentOff();
    // present_mode = EModes::TIMER;
    busy_now = false;
    if (sensor_error){      // センサーエラーでターミネートされたら異常終了

        single_last_meas = false;   //１回計測用のフラグをクリア
        single_meas_counter = 0;
        finished_single_meas = false; 

        failed_meas = true; // エラー通知
        setVmonFailed();
    }
    return;
}

/// @brief 電圧を読み取る
/// @return 電圧値[/uV]
uint32_t Measurement::read_voltage(void){
    occupy_the_bus = true;
    delay(100);// 計測に必要な時間のダミー
    if(DEBUG){Serial.print("RVol ");}
    occupy_the_bus = false;
    return 2222;
}

/// @brief 電流を読み取る
/// @return 電流値[/uA]
uint32_t Measurement::read_current(void){
    occupy_the_bus = true;
    // delay(100);// 計測に必要な時間のダミー
    if(DEBUG){Serial.print("RCur ");}

    float results = 0.0;
    // float readout = 0.0;
    uint32_t readout = 0;

    // これは毎回やる必要ない。インスタンス化した時に１回やって、係数を決めてしまう。
    // 
    float adc_gain_coeff = 0.0;
    switch (meas_adc->getGain()){
      case adsGain_t::GAIN_TWOTHIRDS:
        adc_gain_coeff = ADC_READOUT_VOLTAGE_COEFF::GAIN_TWOTHIRDS;
        break;

      case adsGain_t::GAIN_ONE:
        adc_gain_coeff = ADC_READOUT_VOLTAGE_COEFF::GAIN_ONE;
        break;

      default:
        adc_gain_coeff = ADC_READOUT_VOLTAGE_COEFF::GAIN_TWO;
        break;
    }
    // ここまで

    Serial.print("Current Meas: read_voltage(2-3): ");
    
    for (uint16_t i = 0; i < ADC_AVERAGE_DEFAULT; i++){
        uint16_t temp = (meas_adc->readADC_Differential_2_3() - p_parameter->adc_OFS_comp_diff_2_3);
        Serial.print(", "); Serial.print(temp);  
        readout += temp;
    }

    Serial.print(":conveted to Current Out(2-3):");
    // results = results / (float)avg * coeff * ADC_ERR_COMPENSATION; // reading in microVolt
    results = (float)readout / (float)ADC_AVERAGE_DEFAULT * adc_gain_coeff * p_parameter->adc_err_comp_diff_2_3; // reading in microVolt
    results = results / (float)CURRENT_MEASURE_COEFF; // convert voltage to current.
    Serial.print(results);
    Serial.println(" uA: Fin. --");
    

    occupy_the_bus = false;
    return round(results);
    // return 5555;
}

/// @brief 液面計測を実行
/// @param void 
/// @return uint16_t 液面 [0.1%] 
uint16_t Measurement::read_level(void){
    occupy_the_bus = true;
    uint32_t voltage = read_voltage();
    uint32_t current = read_current();
    // レベルの計算・補正
    occupy_the_bus = false;
    return 500;
}

