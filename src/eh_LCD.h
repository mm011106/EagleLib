/**************************************************************************/
/*!
 * @file Eh_Lcd.h
 * @brief  
 * @author Masakazu Miyamoto
 * @date 2023/7/16
 * $Version:    0.1
 * @par History 2023/7/16 coding start
 *              
 *      
 * 
 */
/**************************************************************************/

#ifndef _EhLcd_H_
#define _EhLcd_H_

#include <Arduino.h>
#include <rgb_lcd.h>


/*!
 * @class   EhLcd
 * @brief   
 *          
 * 
 */

/// @brief 
class EhLcd : public rgb_lcd {

    public:

    // Class 定数
        // DEBUGモードフラグ
        static constexpr bool DEBUG = true;

        // 表示モード 
        static constexpr bool BLINK_MODE = true;
        static constexpr bool HOLD_MODE = false;

        // 表示項目ごとのプロパティに名前でアクセスするための列挙型
        // 
        enum class EDisplayItemName {
            MODE = 0,
            TIMER_IND,
            TIME_REMAIN,
            LEVEL,
            SENSOR,
            BARGRAPH
        };

        enum class EModes{MANUAL = 0, TIMER, CONTINUOUS};
        const char ModeInd[3]={'M','T','C'};
    
    // ファンダメンタルな関数

        EhLcd(void){
        };
    
        ~EhLcd(){
        };

        void clk_in(void);
        bool acquire(void);
        void free(void);
        bool isReady(void);
        bool isAvailable(void);

    // 機能としてのclass関数の定義 
    
        bool init(void);
        void showSplash(const char *message);
        void showHardwareError(const uint8_t error_code);
        void activateDisplay(const bool active);

        // 表示アイテムの直接操作

        void setBlink(const EDisplayItemName item, const bool mode);
        void setVisible(const EDisplayItemName item, const bool visible);
        void setText(const EDisplayItemName item, const String Value);

        // 表示内容を数値で指定するセッター

        bool setLevel(const uint16_t value);
        bool setBargraph(const uint16_t value);
        void setSensorlength(const uint8_t value);
        void setTimerperiod(const uint8_t value);
        void setTimerRemain(const uint8_t value);
        void setMeasMode(const EModes meas_mode);

        //  その他

        void setError(const bool error);
        void writeFrame(void);
        void setVacateI2Cbus(const bool flag);

        // uint8_t itemName2Int(EDisplayItemName itemname){
        //     return static_cast<uint8_t>(itemname);
        // }


    private:
   
    // 定数
        // リソース解放命令後の解放までの遅延時間設定（デフォルト）[CLKサイクル]
        static constexpr uint16_t DEFAULT_KEEP_BUSY = 2; //[CLK Cycle]

        /*
        *   その他定数 
        */
        static constexpr uint8_t DEFAULT_BLINK_PERIOD = 5; //[CLK Cycle]

        //  バーグラフのためのCGデータ 
        static constexpr uint8_t cg_count = 5;
        static constexpr uint8_t cg_y_dots = 8;
        uint8_t bar_graph[cg_count][cg_y_dots] = {
        {       // 0x01: |
            0b10000,
            0b10000,
            0b10000,
            0b10000,
            0b10000,
            0b10000,
            0b10000,
            0b10000 
        }, {    // 0x02: ||
            0b11000,
            0b11000,
            0b11000,
            0b11000,
            0b11000,
            0b11000,
            0b11000,
            0b11000
        }, {    // 0x03: |||
            0b11100,
            0b11100,
            0b11100,
            0b11100,
            0b11100,
            0b11100,
            0b11100,
            0b11100
        }, {    // 0x04: ||||
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110
        }, {    // 0x05: |||||
            0b11111,
            0b11111,
            0b11111,
            0b11111,
            0b11111,
            0b11111,
            0b11111,
            0b11111
        },
    };


    // 構造体  
        // 表示要素のプロパティ
        
        /// @brief 表示要素のプロパティ
        /// @note 20byte/element
        struct ItemProperty{   
            bool mode = false;  // True: Blink False: Normal
            bool state = true;  // True: 表示   False: 非表示 
            uint8_t count = 1; // Blink時のカウンタ 
            uint8_t x_location = 0;  // 表示位置X
            uint8_t y_location = 0;  // 表示位置y
            String text = "";   // 表示内容
            bool refresh = true;   // 当該項目の表示内容をリフレッシュするかどうか
        };

    // 変数
    //  Class instance の占有状態・内部状態を示すフラグ 

        // @brief  リソースが命令実行中
        bool busy_now = false;

        // @brief リソースの占有の状態
        bool resource_available = true; 

        // @brief  リソースの占有を指示するフラグ
        bool acquire_resource = false;

        // @brief  リソースのリリースを指示するフラグ
        bool free_resorce = false;

        //  @brief 表示処理にクロックをつなぐ＝CLKを有効にする
        bool enable_CLK = false;

        // @brief I2Cバスを解放する
        bool vacateI2Cbus = false;
        
        /*!
         * @brief リソースをリリースした後も
         *        設定された時間だけビジーステートを有効にするための
         *        CLK用カウンタ
         *        [CLK count]
         */
        uint16_t keep_busy = DEFAULT_KEEP_BUSY;

        // 時分割のスレット番号をカウントする
        uint16_t thread_count = 0;

    // 内部で保持する表示内容   外部からsetされるのでその表示順が来るまで維持しておくため
    // 
        /*!
        * @brief 内部で保持する表示内容: センサ長 
        */
        String sensor_length = "";
        
        /*!
        * @brief 内部で保持する表示内容: タイマ周期 
        */
        String timer_period = "";


    // 
    // 内部信号
        //  変化する表示内容 
        static const size_t item_count = 6;  // display_itemsの要素数

        // 全ての表示要素のプロパティ
        ItemProperty display_items[item_count]={
            // 0:MODE表示 
            {   .mode = false,
                .state = true,
                .count = DEFAULT_BLINK_PERIOD,
                .x_location = 0, .y_location = 0,
                .text = ""    
            },
            // 1: タイマ動作中表示
            {   .mode = false,
                .state = true,
                .count = DEFAULT_BLINK_PERIOD,
                .x_location = 1, .y_location = 0,
                .text = ":"
            },
            // 2: タイマ経過時間表示
            {   .mode = false,
                .state = true,
                .count = DEFAULT_BLINK_PERIOD,
                .x_location = 2, .y_location = 0,
                .text = ""
            },
            // 3: 液面表示（数値）
            {   .mode = false,
                .state = true,
                .count = DEFAULT_BLINK_PERIOD,
                .x_location = 10, .y_location = 1,
                .text = ""
            },
            // 4: センサ長／エラー表示
            {   .mode = false,
                .state = true,
                .count = DEFAULT_BLINK_PERIOD,
                .x_location = 0, .y_location = 1,
                .text = ""
            },
            // 5: 液面表示（バーグラフ）
            {   .mode = false,
                .state = true,
                .count = DEFAULT_BLINK_PERIOD,
                .x_location = 10, .y_location = 0,
                .text = ""
            }
        };


    // 変更されない表示の記述
        static const size_t frame_item_count = 4;
        ItemProperty frame[frame_item_count]={
            // 0: タイマ周期表示
            {   .mode = false,
                .state = true,
                .count = DEFAULT_BLINK_PERIOD,
                .x_location = 5, .y_location = 0,
                .text = ""
            },
            // 1: タイマの経過時間／周期の区切り（スラッシュ）
            {   .mode = false,
                .state = true,
                .count = DEFAULT_BLINK_PERIOD,
                .x_location = 4, .y_location = 0,
                .text = "/"
            },
            // 2: レベル    バー表示のフレーム
            {   .mode = false,
                .state = true,
                .count = DEFAULT_BLINK_PERIOD,
                .x_location = 8, .y_location = 0,
                .text = "E:    :F"
            },
            // 3: レベル    数値表示    ％
            {   .mode = false,
                .state = true,
                .count = DEFAULT_BLINK_PERIOD,
                .x_location = 15, .y_location = 1,
                .text = "%"
            }     
            
        };


    // private関数

    void writeItem(ItemProperty& item);

    // arrayのサイズを計算
    template < typename TYPE, size_t SIZE >
        size_t array_length(const TYPE (&)[SIZE]){
            return SIZE;
        }


};



#endif //_EhLcd_H_
