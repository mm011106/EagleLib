#include "statemachine.h"


/*!
    * @brief 状態遷移のためのトリガ信号を与えます
    * @param  Statemachine::E_Transit型   状態遷移のための信号名
    * @return true: statusが更新された false: 更新なし
    * @note 計測コマンド(command)もこの中でセットされる
    */
bool Statemachine::setTransitSignal(const E_Transit signal){

  if(DEBUG){Serial.print("(Signal:");}
  previous_machine_status = machine_status;

  switch (signal) {
    //  スイッチのクリック
    case E_Transit::CLICK :
        if(DEBUG){Serial.print("CLICK)");}
        if(machine_status == E_Status::TIMER){
            machine_status = E_Status::MANUAL;
            command = E_MeasCommand::START;
        }
        else if(machine_status == E_Status::CONT){
            machine_status = E_Status::TIMER;
            command = E_MeasCommand::STOP;
        } else {
            command = E_MeasCommand::IDLE;
        }
        break;

    //  スイッチの長押し
    case E_Transit::LONG :
        if(DEBUG){Serial.print("LONG)");}
        if (machine_status == E_Status::TIMER){
            machine_status = E_Status::CONT;
            command = E_MeasCommand::START;
        } else if (machine_status == E_Status::CONT){
            machine_status = E_Status::TIMER;
            command = E_MeasCommand::STOP;
        } else {
            command = E_MeasCommand::IDLE;
        }   
        break;

    //  タイマのタイムアップ
    case E_Transit::TIMEUP :
        if(DEBUG){Serial.print("TIMEUP)");}
        if(machine_status == E_Status::TIMER){
            machine_status = E_Status::MANUAL;
            command = E_MeasCommand::START;
        } else {
            command = E_MeasCommand::IDLE;
        }   
        break;

    //  測定完了（一回計測で計測が完了した場合に発生する信号）
    case E_Transit::MEASCPL :
        if(DEBUG){Serial.print("MEAS end)");}
        if(machine_status == E_Status::MANUAL){
            machine_status = E_Status::TIMER;
            command = E_MeasCommand::STOP;
        } else {
            command = E_MeasCommand::IDLE;
        }   
        break;

    //  測定エラー  計測が開始できなかった場合に発生
    case E_Transit::MEASERR :
        if(DEBUG){Serial.print("MEAS ERR!)");}
        machine_status = E_Status::TIMER;
        command = E_MeasCommand::STOP;
        break;

    //  現状維持    （通常は使わない）
    case E_Transit::IDLE :
        if(DEBUG){Serial.print("IDLE)");}
        command = E_MeasCommand::IDLE;
    break;

    default:
        command = E_MeasCommand::IDLE;
    break;
    }
  
    updated = (previous_machine_status != machine_status);
    
    if(DEBUG && updated){
        Serial.print(" --Update to: ");
        Serial.print(getStatusChar());
        Serial.print(". COMMAND=");
        Serial.println(static_cast<uint8_t>(getMeasCommand()));
    }

    return (updated);
}

/*!
* @brief 状態遷移が行われたかどうかを返します
* @return true: statusが更新された false: 更新なし
* @note 読み取るたびに値はクリアされます
*/
bool Statemachine::hasStatusUpdated(void){
    bool temp_flag = updated;
    updated = false;
    return temp_flag;
};

/// @brief 状態遷移に応じた計測部への指示を返します
/// @return E_MeasCommand型 測定命令
/// @note hasStatusUpdated()==true 時に有効になります
Statemachine::E_MeasCommand Statemachine::getMeasCommand(void){
    return command;
}

/*!
* @brief マシンの内部状態を返します
* @param  
* @return E_Status型 
*/
Statemachine::E_Status Statemachine::getStatus(void){
    return machine_status;
};

/// @brief ステートマシンの状態に応じた文字を返します（表示用）
/// @param  void
/// @return ModeIndで定義した文字の中の1文字が状態に応じて返されます
char Statemachine::getStatusChar(void){
    return ModeInd[static_cast<uint8_t>(machine_status)];
}
