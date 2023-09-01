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
    // ！！要検討！！   6インチぐらいの短いセンサだと伝搬時間内に３回測定できない
    //      0.5秒ぐらいのLowlimitを設ける
    if (single_meas_period < 150){ // 1.5秒より短い伝搬時間の場合
        single_meas_interval = 50; // 0.5s周期
    } else{
        single_meas_interval = single_meas_period/3;//[CLK count]   
    }

    if(DEBUG){Serial.println(sensor_heat_propagation_time);}
    if(DEBUG){Serial.print("Sensor Length:"); Serial.println(p_parameter->sensor_length);}
    if(DEBUG){Serial.print("Delay Time:"); Serial.println(sensor_heat_propagation_time);}
    if(DEBUG){Serial.print("Sensor R:"); Serial.println(sensor_resistance);}
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
    switch (command)
    {
    case Measurement::ECommand::SINGLE :
        if (!busy_now){
            present_mode = EModes::MANUAL;
            busy_now = true;
            if (currentOn()){
                if(DEBUG){Serial.print("SINGLE Start. ");Serial.println(micros());}
                single_meas_counter = 0;
                sensor_error = false;
            }else{
                if(DEBUG){Serial.println("SINGLE Meas ERROR. terminate");}
                sensor_error = true;
                terminateMeasurement();
            }
        } else {
            if(DEBUG){Serial.println("Fail: measure command while busy.");}
        }
        break;

    case Measurement::ECommand::CONTSTART :
        if(!busy_now){
            present_mode = EModes::CONTINUOUS;
            busy_now = true;
            if (currentOn()){
                if(DEBUG){Serial.println("CONT Start.");}
                sensor_error = false;
            } else {
                if(DEBUG){Serial.println("CONT Meas ERROR. terminate");}
                sensor_error = true;
                terminateMeasurement();
            }
        } else {
            if(DEBUG){Serial.println("Fail: measure command while busy.");}
        }
        break;
    
    case Measurement::ECommand::CONTEND :
        if(busy_now){
            terminateMeasurement();
        } else {
            if(DEBUG){Serial.println("Fail: CONT END command while ready.");}
        }
        break;

    case Measurement::ECommand::TERMINATE :
        if(busy_now){
            if(DEBUG){Serial.println("TERMINATE.");}
            terminateMeasurement();
        } else {
            if(DEBUG){Serial.println("Fail: TERMINATE command while ready.");}
        }

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

    uint16_t result = 0;
    if (getCurrentSourceStatus()){
        sensor_error = false;
        result = read_level();
    } else {
        sensor_error = true;
    }

    // Sensorエラー処理（異常終了）
    if (sensor_error){
        if(DEBUG){Serial.print("-sensorError  - ");}
        //センサエラー（測定中にエラー発生）なら計測を終了して帰る
        if(DEBUG){Serial.println("Measurement Treminate by error.");}
        terminateMeasurement();
        
        // Vmonにエラーを出力
        void setVmonFailed(void);
        return;
    }

    // 一回計測の最終計測の場合は一回計測のクロージング処理
    if (the_last_meas){
        if(DEBUG){Serial.print("-single:last- ");}
        single_last_meas = false;
        single_meas_counter = 0;
        should_measure = false;//最終計測なので、計測中に入った測定要求は無視する
        terminateMeasurement();
    }

    // 測定結果を出力する処理
    // 電圧モニタへの出力
    setVmon(result);
    // LCD,UART向けの出力方法を考える
    // フラグを出力(フラグ、getter必要)
    // 結果を出力（getter必要）

    if(DEBUG){Serial.println("execMeas::End ");}
}

/*!
 * @brief 電流源をonにする
 */
bool Measurement::currentOn(void){
    if(DEBUG){Serial.println("CurrentSoruce ON");}
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
    if(DEBUG){Serial.println("CurrentSoruce OFF");}
    return ;
}

/*!
 * @brief 電流源を設定する
 * @param current 設定電流値[0.1mA]
 */
void Measurement::setCurrent(uint16_t current){
    if(DEBUG){Serial.print("CurrentSoruce set");Serial.println(current);}
    return;
}

/*!
 * @brief 電流源のステータスを返す
 * @returns True: 電流Onの設定で正常に電流を供給している, False:電流がoff もしくは 負荷異常
 */
bool Measurement::getCurrentSourceStatus(void){
    if(DEBUG){Serial.print("C-C ");}
    return true;
    // // FOR TESST
    // return false;
    // long rand = random(100);
    // if (rand > 20){
    //     return true;
    // }else {
    //     return false;
    // }
}

/*!
 * @brief 測定を終了する
 */
void Measurement::terminateMeasurement(void){
    should_measure = false;
    currentOff();
    present_mode = EModes::TIMER;
    busy_now = false;
    return;
}

/// @brief 液面計速を実行
/// @param void 
/// @return uint16_t 液面 [0.1%] 
uint16_t Measurement::read_level(void){
    delay(150);// 計測に必要な時間のダミー
    if(DEBUG){Serial.print("RL ");}
    return 1234;
}

/// @brief 電圧モニタ出力を設定する 
/// @param vout 出力電圧[0.1V] 
void Measurement::setVmon(uint16_t vout){
    if(DEBUG){Serial.print("Vout: set");Serial.println(vout);}
    return;
}

/// @brief  電圧モニタ出力にエラーを提示する（0V) 
/// @param  void
void Measurement::setVmonFailed(void){
    if(DEBUG){Serial.println("Vout: Error indicate.");}    
}