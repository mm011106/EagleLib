/**************************************************************************/
/*!
 * @file EH_LED.h
 * @brief  LEDドライブ用クラス
 * @author Masakazu Miyamoto
 * @date 2022/6/23-
 * $Version:    1.1$
 * @par History
 *      2023/6/23   コーディング開始
 *      2023/6/28   V1.0完成
 *      2023/8/22   V1.1 占有処理など見直し
 *      
 */
/**************************************************************************/

#ifndef _EH_LED_H_
#define _EH_LED_H_

#include <Arduino.h>

/*!
 * @class   EH_LED
 * @brief   LEDの点灯を管理するクラス
 *          
 * 
 */

class EH_LED {
    public:
        
        /*!
         * @brief LEDの点灯モードを列挙
         *          CONT=0
         *          BLINK=1
         */
        enum class E_IllumiMode{
            CONT,       //  0:連続
            BLINK,      //  1:点滅
        };

        /*!
         * @brief constructor   ポート番号を指定
         * @param port_number:  LEDが接続されているポート番号（Arduino名もしくはSTM32ポート番号_ALT)
         * 
         */
        EH_LED(uint32_t port_number){
            LED_port_number = port_number;
            pinMode(LED_port_number, OUTPUT);
        };
    
        /*!
         * @brief deconstructor
         *  LED点灯で使っていたポートを解放し、入力に設定。
         */
        ~EH_LED(){
            pinMode(LED_port_number, INPUT);
        };


        /*!
         * @brief LEDの状態を読み込みます
         * @param  
         * @return (点灯=1｜消灯=0)*2 + (点滅=1｜連続=0)
         */
        uint16_t getStatus(void){
            return digitalRead(LED_port_number)*2 + static_cast<uint16_t>(LED_mode);
        };

        /*!
         * @brief  LEDが接続されているポートを番号で返します
         * @param  void
         * @return uint32_t ulPin 
         */
        uint32_t getPortNumber(void){
            return LED_port_number;
        };

        /*!
        * @brief LEDを点灯
        * @param  void
        * @return   true:指示受付   false:指示却下
        */
        bool turnOn(void){
            // while(busy_now){asm("nop");}
            LED_state=true;
            busy_now = true;
            return true;
        };

        /*! 
        * @brief LEDを消灯
        * @param  void
        * @return   true:指示受付   false:指示却下
        */
        bool turnOff(void){
            // while(busy_now){asm("nop");}
            LED_state=false;
            busy_now = true;
            return true;
        };


        bool setMode(E_IllumiMode mode = DEFAULT_ILLUMI_MODE, uint16_t blink_period = DEFAULT_BLINK_PERIOD  );
        void clk_in(void);

        bool acquire(void);
        void free(void);

        /*!
         * @brief リソースが命令受付可能かどうか
         * @return True:受け入れ可   False:命令実行中
         */
        bool isReady(void){
            return !busy_now;
        };

        /*!
         * @brief リソースを占有できる状態か？
         * @return True:占有可能   False:使用中
         */
        bool isAvailable(void){
            return resource_available;
        };

    private:

        /*
        * 点滅モードの時の点灯／消灯の時間設定（デフォルト）[CLKサイクル]
        */
        static constexpr uint16_t DEFAULT_BLINK_PERIOD = 100;   //[CLK cycle]

        // リソース解放命令後の解放までの遅延時間設定（デフォルト）[CLKサイクル]
        static constexpr uint16_t DEFAULT_KEEP_BUSY = 2; //[CLK Cycle]

        // LEDの点灯モード(デフォルト)
        static constexpr E_IllumiMode DEFAULT_ILLUMI_MODE = E_IllumiMode::CONT;

        // リソースのポート番号
        uint32_t LED_port_number = 0;

        // リソースが命令実行中
        bool busy_now = false;

        // リソースの占有の状態
        bool resource_available = true; 

        // リソースの占有を指示するフラグ
        bool acquire_resource = false;

        // リソースのリリースを指示するフラグ
        bool free_resource = false;

        /*!
         * @brief リソースをリリースした後も
         *        設定された時間だけビジーステートを有効にするための
         *        CLK用カウンタ
         *        [CLK count]
         */
        uint16_t keep_busy = DEFAULT_KEEP_BUSY;

        // LEDの点灯モード設定 
        E_IllumiMode LED_mode = DEFAULT_ILLUMI_MODE;

        // 点滅周期設定[CLK count]
        uint16_t blink_period = 0;

        // 点滅モード時のCLKカウント用
        uint16_t blink_count =0;

        // BLINKモードでの点灯／消灯の状態を示すフラグ
        bool blink_flag = false;

        // 内部でのLED状態を示す True:点灯  False:消灯
        bool LED_state = false;

};



#endif //_EH_LED_H_
