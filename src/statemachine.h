/**************************************************************************/
/*!
 * @file statemachine.h
 * @brief EH-900 動作モード制御用ステートマシン
 * @author Masakazu Miyamoto
 * @date 2022/8/1-
 * $Version:    0.0$
 * @par History
 *    
 * 
 */
/**************************************************************************/

#ifndef _EH_statemachine_H_
#define _EH_statemachine_H_

#include <Arduino.h>

/*!
 * @class   Statemachine
 * @brief   EH-900 動作モード制御用ステートマシン
 *          
 * 
 */
class Statemachine {

    public:
        /// @brief ステートマシンの状態遷移イベント一覧
        enum class ETransit : uint8_t{
            IDLE = 0,
            CLICK,  //  操作スイッチ  クリック
            LONG,   //  操作スイッチ  長押し
            TIMEUP, //  タイマ計測
            MEASCPL //  測定完了
        };

        // @brief ステートマシン状態一覧
        enum class EStatus : uint8_t {
            TIMER = 0,
            MANUAL,
            CONT
        };

        /// @brief モード表示のための文字配列（EStatusと連携）
        const char ModeInd[3]={'T','M','C'};

        enum class EMeasCommand : uint8_t{
            IDLE = 0,
            SINGLE,
            CONTSTART,
            CONTEND
        };

        /*!
         * @brief constructor   
         */
        Statemachine(){
        };
    
        /*!
         * @brief deconstructor
         *  
         */
        ~Statemachine(){
        };


        /*!
         * @brief マシンの内部状態を返します
         * @param  
         * @return EStatus型 
         */
        EStatus getStatus(void){
            return machine_status;
        };

        /// @brief ステートマシンの状態に応じた文字を返します（表示用）
        /// @param  void
        /// @return ModeIndで定義した文字の中の1文字が状態に応じて返されます
        char getStatusChar(void);


        /*!
         * @brief 状態遷移のためのトリガ信号を与えます
         * @param  Etransit型   状態遷移のための信号名
         * @return true: statusが更新された false: 更新なし
         * @note 
         */
        bool setTransitSignal(ETransit signal);

        /*!
         * @brief 状態遷移が行われたかどうかを返します
         * @return true: statusが更新された false: 更新なし
         * @note 読み取るたびに値はクリアされます
         */
        bool hasStatuUpdated(void){
            bool temp_flag = updated;
            updated = false;
            return temp_flag;
        };

        /// @brief 状態遷移に応じた計測部への指示を返します
        /// @return EMeasCommand型 測定命令
        /// @note isChanged()==true 時に有効になります
        EMeasCommand getMeasCommand(void);
        

    private:

        EStatus machine_status = EStatus::TIMER;
        EStatus previous_machine_status = EStatus::TIMER;

        bool updated = false;

        EMeasCommand command = EMeasCommand::IDLE;


};



#endif //_EH_statemachine_H_
