#include "eh_LCD.h"


/*! 
@brief  LCDイニシャライズ、スプラッシュ表示、エラー表示
@param error エラーコード（エラー表示のため）
@return True:正常終了   False:パラメタ参照用のeh900型変数が未定義
*/
bool EhLcd::init(uint8_t error){

    rgb_lcd::begin(16,2);
    delay(10);  // delayを入れないとうまく通信できない時がある

    // CGRAM読み込み    beginのすぐ後に実行する
    // CGaddress = 0 は表示関数でNULLと判断されるので使わない
    for (uint8_t i=0; i<cg_count; i++){
        rgb_lcd::createChar(i+1, bar_graph[i]);
    }

    rgb_lcd::clear();
    delay(10);

    // スプラッシュ表示
    rgb_lcd::setCursor(2, 0);
    rgb_lcd::print("-- EH-900 --");
    rgb_lcd::setCursor(1, 1);
//     rgb_lcd::print(REV);

    delay(1000);
    rgb_lcd::clear();
    delay(10);

    // ハードウエアエラー表示のコードを書く（エラーコードを引数にいれる）

// for TEST
    Serial.print("- eh_LCD.ino");
    Serial.println(sizeof(display_items));
// -------


    return true;
};

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
    if(now_thread==0){
//         digitalWrite(port_longPress,HIGH);
//         digitalWrite(port_longPress,LOW);
    }

    // display_itemに記載のある表示内容を表示
    if (now_thread < item_count && enable_CLK ){
        writeItem(&display_items[now_thread]);  //スレッド１つ当たり１つの項目を表示させる
    
        // blinkの処理
        if (display_items[now_thread].mode == BLINK_MODE){
            if ( --display_items[now_thread].count == 0){
                display_items[now_thread].count = DAFAULT_COLON_BLINK_PERIOD;
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
void EhLcd::writeItem(ItemProperty *item){

    if (item->refresh){  //表示更新の指示がある場合表示を更新
        rgb_lcd::setCursor(item->x_location,item->y_location);
        
        String buffer = "";

        //  表示・非表示の処理  非表示の場合、表示されていた内容をスペースで消去
        if(item->state){
            buffer = item->text;
        } else {
            for (uint8_t i=0 ; i < item->text.length() ; i++){
                buffer = buffer + " ";
            }
        }
        rgb_lcd::print(buffer);
        // new imprementation !!!
        item->refresh = false;
    }

    return;
}

/*
* 非同期で動作する関数の記述
*/

// 表示アイテムの直接操作

    /// @brief 表示アイテムの直接操作:点滅モードの設定
    /// @param item：表示項目の指定 EDisplayItemNameの要素名で指定
    /// @param mode True=Blink, False=Hold
    void EhLcd::setBlink(const EDisplayItemName item, const bool mode){
        display_items[static_cast<uint8_t>(item)].mode = mode;
        display_items[static_cast<uint8_t>(item)].state = true;
        display_items[static_cast<uint8_t>(item)].refresh = true;
    };

// 表示値を設定する

    //  LCD表示内容を設定する：液面レベルの設定
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

    //  LCD表示内容を設定する：液面レベルの設定
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


    // LCD表示内容を設定する：タイマ周期の設定
    void EhLcd::setTimerperiod(const uint8_t value){
        String msg = ("  "+String(value));
        timer_period = msg.substring(msg.length()-2);
        frame[0].text = timer_period;
        frame[0].refresh = true;
    };

    //  LCD表示内容を設定する：センサ長の設定
    void EhLcd::setSensorlength(const uint8_t value){
        String msg = ("  "+String(value));
        msg = msg.substring(msg.length()-3);
        sensor_length = msg +"inch";
        display_items[static_cast<uint8_t>(EDisplayItemName::SENSOR)].text = sensor_length;
        display_items[static_cast<uint8_t>(EDisplayItemName::SENSOR)].refresh = true;
    };

    void EhLcd::setTimerRemain(const uint8_t value){
        String msg = ("  "+String(value));
        msg = msg.substring(msg.length()-2);
        display_items[static_cast<uint8_t>(EDisplayItemName::TIME_REMAIN)].text=msg;
        display_items[static_cast<uint8_t>(EDisplayItemName::TIME_REMAIN)].refresh = true;
    };

    void EhLcd::setMeasMode(const EModes meas_mode){
        display_items[static_cast<uint8_t>(EDisplayItemName::MODE)].text = ModeInd[static_cast<uint8_t>(meas_mode)];
        display_items[static_cast<uint8_t>(EDisplayItemName::MODE)].refresh = true;
    }


//  エラー表示
    void EhLcd::setError(bool error){
        String msg = "";
        if (error){
            msg = "-ERROR-";
            display_items[static_cast<uint8_t>(EDisplayItemName::SENSOR)].mode = BLINK_MODE;
        } else {
            msg = sensor_length;
            display_items[static_cast<uint8_t>(EDisplayItemName::SENSOR)].mode = HOLD_MODE;
        }
        display_items[static_cast<uint8_t>(EDisplayItemName::SENSOR)].text = msg;
        display_items[static_cast<uint8_t>(EDisplayItemName::SENSOR)].refresh = true;
    }


// フレームの表示   非同期処理（即時書き込み）
    void EhLcd::writeFrame(void){
        for (uint8_t i=0 ; i<frame_item_count ; i++){
            writeItem(&frame[i]);
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
* @param  void
* @return void
* @note 指定時間(DEFAULT_KEEP_BUSY)経過後解放される
*/
void EhLcd::free(void){
    free_resorce=true;
    return;
};
