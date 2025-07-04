// コンパイル方法
// g++ send_command_with_crc.cpp -lboost_system -lpthread

// boostインストール方法
// sudo apt update
// sudo apt install libboost-all-dev


#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <sstream>
#include <iomanip>

// CRC8-MAXIM calculation
// Polynomial: x8 + x5 + x4 + 1 (0x31)
uint8_t crc8_maxim(const uint8_t* data, size_t length) {
    uint8_t crc = 0x00;
    const uint8_t polynomial = 0x31; // x8 + x5 + x4 + 1
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}


int main(void) {
    // io_contextを作成
    boost::asio::io_context io_context; 
    // シリアルポート開ける
    boost::asio::serial_port port(io_context, "/dev/ttyACM0"); 

    // ポートの設定
    port.set_option(boost::asio::serial_port_base::baud_rate(115200));
    port.set_option(boost::asio::serial_port_base::character_size(8));
    port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
    port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));

    std::vector<uint8_t> data = {
        0x01,
        0x64,
        0x00,
        0x64,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x4F
    };
    

    // コマンドを送る
    boost::asio::write(port, boost::asio::buffer(data, data.size()));

    // // 受信バッファを準備（10バイト）
    // std::vector<uint8_t> feedback(10);

    // // 実際に受信
    // boost::asio::read(port, boost::asio::buffer(feedback, feedback.size()));
    
    // // 受信したデータを16進数で表示
    // std::cout << "フィードバック: ";
    // for (size_t i = 0; i < feedback.size(); i++) {
    //     std::cout << std::hex << std::setfill('0') << std::setw(2) 
    //               << static_cast<int>(feedback[i]);
    //     if (i < feedback.size() - 1) std::cout << " ";
    // }
    // std::cout << std::endl;
    
    // // CRC8の検証
    // uint8_t calculated_crc = crc8_maxim(feedback.data(), 9);
    // bool crc_valid = (calculated_crc == feedback[9]);
    
    // std::cout << "CRC8チェック: " << (crc_valid ? "OK" : "NG") << std::endl;

    // // デバッグ出力
    // if (crc_valid) {
    //     uint8_t motor_id = feedback[0];
    //     uint8_t mode_value = feedback[1];
    //     int16_t torque_current = (feedback[2] << 8) | feedback[3];
    //     int16_t velocity = (feedback[4] << 8) | feedback[5];
    //     uint8_t temperature = feedback[6];
    //     uint8_t position_u8 = feedback[7];
    //     uint8_t error_code = feedback[8];
        
    //     std::cout << "\nResponse" << std::endl;
    //     std::cout << "  Motor ID: " << static_cast<int>(motor_id) << std::endl;
    //     std::cout << "  Mode: " << static_cast<int>(mode_value) << std::endl;
    //     std::cout << "  Torque Current: " << torque_current << std::endl;
    //     std::cout << "  Velocity: " << velocity << " rpm" << std::endl;
    //     std::cout << "  Temperature: " << static_cast<int>(temperature) << "°C" << std::endl;
    //     std::cout << "  Position (0-360°): " << static_cast<int>(position_u8) * 360 / 255 << "°" << std::endl;
    //     std::cout << "  Error Code: 0x" << std::hex << static_cast<int>(error_code) << std::endl;
    // } else {
    //     std::cout << "CRC8ミスマッチ." << std::endl;
    // }

    return 0;
}
