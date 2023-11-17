#include "eh_LCD.h"


/*! 
@brief  LCDイニシャライズ
@return True:常に正常終了
@note   clk_in()の動作を始める前に呼び出す必要あり
*/
bool EhLcd::init(void){

    rgb_lcd::begin(16,2);
    delay(10);  // delayを入れないとうまく通信できない時がある

    // CGRAM読み込み    beginのすぐ後に実行する
    // CGaddress = 0 は表示関数でNULLと判断されるので使わない
    for (uint8_t i=0; i<cg_count; i++){
        rgb_lcd::createChar(i+1, bar_graph[i]);
    }

    if (DEBUG){
        Serial.print("- eh_LCD.ino");
        Serial.println(sizeof(display_items));
        Serial.println(sizeof(frame));
    }

    return true;
};

/// @brief 起動時のメッセージ（型式等）を表示します。
/// @param message char配列のポインタ   16文字までのメッセージを指定
void EhLcd::showSplash(const char *message){
    rgb_lcd::clear();
    delay(10);

    rgb_lcd::setCursor(2, 0);
    rgb_lcd::print("-- EH-900 --");
    rgb_lcd::setCursor(0, 1);
    rgb_lcd::print(message);
    delay(2000);

    rgb_lcd::clear();
    delay(300);
    return;
}


/// @brief 起動時のハードウエアチェックのエラー表示
/// @param error_code この数値をそのまま表示します
void EhLcd::showHardwareError(const uint8_t error_code){
    rgb_lcd::clear();
    delay(10);

    rgb_lcd::setCursor(0, 0);
    rgb_lcd::print("HARDWARE ERROR!");
    rgb_lcd::setCursor(3, 1);
    rgb_lcd::print("#");
    rgb_lcd::print(error_code);

    return;
}

/*!
 * @brief CLKに同期した処理を実行 
 *          スイッチの押されている可動化の判断と押された時間の判断
 *          リソースの占有・解放の処理
 */
void EhLcd::clk_in(void){
    busy_now = true;
 
    //  CLKに同期した処理を記載
 
    uint8_t now_thread = thread_count;

    // 処理時間測定用   フレーム同期信号
    if(DEBUG){
        if(now_thread==0){
            digitalWrite(PA3,HIGH);
        } else {
            digitalWrite(PA3,LOW);
        }
    }

    // display_itemに記載のある表示内容を表示
    if (now_thread < item_count && enable_CLK ){
        writeItem(display_items[now_thread]);  //スレッド１つ当たり１つの項目を表示させる
    
        // blinkの処理
        if (display_items[now_thread].mode == BLINK_MODE){
            if ( --display_items[now_thread].count == 0){
                display_items[now_thread].count = DEFAULT_BLINK_PERIOD;
                display_items[now_thread].state = !display_items[now_thread].state;
                display_items[now_thread].refresh = true;
                // 表示更新
            }
        }
    }

    // リソースの占有解放、busy信号の作成
    if(acquire_resource){
        busy_now = false; // 占有処理完了としてbusyをネゲート
        acquire_resource = false;
    }

    if(free_resorce){
        if(--keep_busy==0){
            resource_available=true;
            busy_now = true;//  解放されたことで動作できなくなるので、busyをアサート
            free_resorce=false;
            keep_busy=DEFAULT_KEEP_BUSY;
            return;
        }
    }

    // スレッドカウンタ 0~9 10スレッド
    if (++thread_count == 10){
        thread_count = 0;
    }

    // 同期信号処理のクロージング処理
    busy_now = false;

    return;
};

/// @brief 同期動作で呼び出す関数:  指定項目の内容をLCDに書く
/// @param itemの(ItemProperty型) 表示項目の指定
void EhLcd::writeItem(ItemProperty& item){

    //表示更新の指示がある場合表示を更新、I2Cバスを使えない時は更新しない
    if (item.refresh && !vacateI2Cbus){  
        rgb_lcd::setCursor(item.x_location,item.y_location);
        
        String buffer = "";

        //  表示・非表示の処理  非表示の場合、表示されていた内容をスペースで消去
        if(item.state){
            buffer = item.text;
        } else {
            for (uint8_t i=0 ; i < item.text.length() ; i++){
                buffer = buffer + " ";
            }
        }
        rgb_lcd::print(buffer);
        // new imprementation !!!
        item.refresh = false;
    }

    return;
}

/*
* 非同期で動作する関数の記述
*/


/// @brief 表示処理を有効／無効にする
/// @param active true:有効 false:無効
void EhLcd::activateDisplay(const bool active){
    enable_CLK = active;
};

/// @brief  I2Cバスの解放指示フラグのセット
/// @param flag 解放指示フラグ True:解放する    False:使用する
void EhLcd::setVacateI2Cbus(const bool flag){
    vacateI2Cbus = flag;
}

// 表示アイテムの直接操作

/// @brief 表示アイテムの直接操作:点滅モードの設定
/// @param item：表示項目の指定 EDisplayItemNameの要素名で指定
/// @param mode True=Blink, False=Hold
void EhLcd::setBlink(const EDisplayItemName item, const bool mode){
    display_items[static_cast<uint8_t>(item)].mode = mode;
    display_items[static_cast<uint8_t>(item)].state = true;
    display_items[static_cast<uint8_t>(item)].refresh = true;
};

/// @brief 表示・非表示の設定
/// @param item：表示項目の指定 EDisplayItemNameの要素名で指定
/// @param visible True=表示, False=非表示
void EhLcd::setVisible(const EDisplayItemName item, const bool visible){
    display_items[static_cast<uint8_t>(item)].state = visible;
};

/// @brief 表示テキストの直接書き込み（非推奨）
/// @param item：表示項目の指定 EDisplayItemNameの要素名で指定
/// @param text 表示テキスト(String)
void EhLcd::setText(const EDisplayItemName item, const String& text){
    display_items[static_cast<uint8_t>(item)].text = text;
};



// 表示値を設定する

/// @brief LCD表示内容を設定する：液面レベルの設定
/// @param value 液面レベル（0.1%単位）
/// @return 常にTrue
bool EhLcd::setLevel(const uint16_t value){
    if (value < 0 | value > 1000){  // 引数のオーバーレンジを検出
        return false;
    }

    String msg = ("  "+String((float(value)/10.0),1));
    msg = msg.substring(msg.length()-5);
    display_items[static_cast<uint8_t>(EDisplayItemName::LEVEL)].text=msg;
    display_items[static_cast<uint8_t>(EDisplayItemName::LEVEL)].refresh = true;

    return true;
};

/// @brief LCD表示内容を設定する：液面レベルの設定（バーグラフ）
/// @param value 液面レベル（0.1%単位）
/// @return 常にTrue
bool EhLcd::setBargraph(const uint16_t value){
    if (value < 0 | value > 1000){  // 引数のオーバーレンジを検出
        return false;
    }

    uint8_t fine = (value % 250)/50;
    uint8_t coarse = value / 250;
    uint8_t index = 0;
    char temp[5]="    ";    // 実際は4文字使うが1文字多く宣言する必要あり。

    // block of 25%
    for (index=0; index<coarse; index++){
        temp[index]=(unsigned char)0x05;
    };
    // block of 5%
    if (fine>0){
        temp[index]=(unsigned char)(fine);
    };
    
    display_items[static_cast<uint8_t>(EDisplayItemName::BARGRAPH)].text=temp;
    display_items[static_cast<uint8_t>(EDisplayItemName::BARGRAPH)].refresh = true;

    return true;
};

/// @brief LCD表示内容を設定する：タイマ周期の設定
/// @param value タイマ周期（分）
void EhLcd::setTimerperiod(const uint8_t value){
    String msg = ("  "+String(value));
    timer_period = msg.substring(msg.length()-2);
    frame[0].text = timer_period;
    frame[0].refresh = true;
};

/// @brief LCD表示内容を設定する：センサ長の設定
/// @param value センサ長(インチ)
void EhLcd::setSensorlength(const uint8_t value){
    String msg = ("  "+String(value));
    msg = msg.substring(msg.length()-3);
    sensor_length = msg +"inch";
    display_items[static_cast<uint8_t>(EDisplayItemName::SENSOR)].text = sensor_length;
    display_items[static_cast<uint8_t>(EDisplayItemName::SENSOR)].refresh = true;
};

/// @brief LCD表示内容を設定する：タイマ残り時間
/// @param value タイマ残り時間
void EhLcd::setTimerRemain(const uint8_t value){
    String msg = ("  "+String(value));
    msg = msg.substring(msg.length()-2);
    display_items[static_cast<uint8_t>(EDisplayItemName::TIME_REMAIN)].text=msg;
    display_items[static_cast<uint8_t>(EDisplayItemName::TIME_REMAIN)].refresh = true;
};

/// @brief LCD表示内容を設定する：計測モード
/// @param meas_mode 測定モード
void EhLcd::setMeasMode(const EModes meas_mode){
    display_items[static_cast<uint8_t>(EDisplayItemName::MODE)].text = ModeInd[static_cast<uint8_t>(meas_mode)];
    display_items[static_cast<uint8_t>(EDisplayItemName::MODE)].refresh = true;
}

/// @brief センサエラーの表示
/// @param error
// True:エラー表示  False:センサ長表示 
void EhLcd::setError(const bool error){
    String msg = "";
    if (error){
        msg = "-ERROR-";
        display_items[static_cast<uint8_t>(EDisplayItemName::SENSOR)].mode = BLINK_MODE;
    } else {
        msg = sensor_length;
        display_items[static_cast<uint8_t>(EDisplayItemName::SENSOR)].mode = HOLD_MODE;
    }
    display_items[static_cast<uint8_t>(EDisplayItemName::SENSOR)].state = true; // BLINK_MODEからHOLD_MODEに戻したら、必ずstateをtrueにすること
    display_items[static_cast<uint8_t>(EDisplayItemName::SENSOR)].text = msg;
    display_items[static_cast<uint8_t>(EDisplayItemName::SENSOR)].refresh = true;
}


/// @brief フレームの表示   非同期処理（即時書き込み）
/// @note 初期化時のみ使用。通常書き換えない表示部分を書き込みます
void EhLcd::writeFrame(void){
    for (uint8_t i=0 ; i<frame_item_count ; i++){
        writeItem(frame[i]);
    }
};

/*!
* @brief リソースを占有する
* @param  bool
* @return True:正常終了    False:現在使用中
*/
bool EhLcd::acquire(void){
    if(resource_available){
        acquire_resource = true;
        busy_now = true;    // 占有処理実行中なのでbusyをアサート
        resource_available = false;// リソースを占有したことを明示
        return true;
    } else {
        return false;
    }
};

/*!
* @brief リソースを解放する
* @note 指定時間(DEFAULT_KEEP_BUSY)経過後解放される
*/
void EhLcd::free(void){
    free_resorce=true;
    return;
};

/*!
    * @brief リソースが命令受付可能かどうか
    * @return True:受け入れ可   False:命令実行中
    */
bool EhLcd::isReady(void){
    return !busy_now;
};

/*!
    * @brief リソースを占有できる状態か？
    * @return True:占有可能   False:使用中
    */
bool EhLcd::isAvailable(void){
    return resource_available;
};

