// コンパイル方法
// g++ send_command_with_crc.cpp -lboost_system -lpthread

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

// 標準入力から16進数文字列を読み取ってベクトルに変換
std::vector<uint8_t> readHexFromInput() {
    std::vector<uint8_t> data;
    std::string line;
    
    std::cout << "16進数を9個入力 (例: 01 64 07 D0 00 00 00 00 00): ";
    std::getline(std::cin, line);
    
    std::istringstream iss(line);
    std::string hexValue;
    
    while (iss >> hexValue && data.size() < 9) {
        try {
            uint8_t byte = static_cast<uint8_t>(std::stoul(hexValue, nullptr, 16));
            data.push_back(byte);
        } catch (const std::exception& e) {
            std::cout << "無効な16進数: " << hexValue << std::endl;
        }
    }
    
    // 9個未満の場合は空のベクトルを返す
    if (data.size() < 9) {
        std::cout << "エラー: " << data.size() << "個しか入力されていません。9個必要です。" << std::endl;
        return std::vector<uint8_t>(); // 空のベクトルを返す
    }
    
    // CRC8を計算して追加
    uint8_t crc = crc8_maxim(data.data(), 9);
    data.push_back(crc);
    
    std::cout << "CRC8自動計算: 0x" << std::hex << std::setfill('0') << std::setw(2) 
              << static_cast<int>(crc) << std::endl;
    
    return data;
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

    // 標準入力から16進数コマンドを読み取り
    std::vector<uint8_t> data = readHexFromInput();
    
    // 入力が不足している場合は終了
    if (data.empty()) {
        std::cout << "終了します" << std::endl;
        return 1;
    }
    
    // 入力されたコマンドを表示
    std::cout << "送信するコマンド: ";
    for (size_t i = 0; i < data.size(); i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) 
                  << static_cast<int>(data[i]);
        if (i < data.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;

    // コマンドを送る
    boost::asio::write(port, boost::asio::buffer(data, data.size()));

    // 受信バッファを準備（10バイト）
    std::vector<uint8_t> feedback(10);

    // 実際に受信
    boost::asio::read(port, boost::asio::buffer(feedback, feedback.size()));
    
    // 受信したデータを16進数で表示
    std::cout << "フィードバック: ";
    for (size_t i = 0; i < feedback.size(); i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) 
                  << static_cast<int>(feedback[i]);
        if (i < feedback.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
    
    // CRC8の検証
    uint8_t calculated_crc = crc8_maxim(feedback.data(), 9);
    bool crc_valid = (calculated_crc == feedback[9]);
    
    std::cout << "CRC8チェック: " << (crc_valid ? "OK" : "NG") << std::endl;

    // デバッグ出力
    if (crc_valid) {
        uint8_t motor_id = feedback[0];
        uint8_t mode_value = feedback[1];
        int16_t torque_current = (feedback[2] << 8) | feedback[3];
        int16_t velocity = (feedback[4] << 8) | feedback[5];
        uint8_t temperature = feedback[6];
        uint8_t position_u8 = feedback[7];
        uint8_t error_code = feedback[8];
        
        std::cout << "\nResponse" << std::endl;
        std::cout << "  Motor ID: " << static_cast<int>(motor_id) << std::endl;
        std::cout << "  Mode: " << static_cast<int>(mode_value) << std::endl;
        std::cout << "  Torque Current: " << torque_current << std::endl;
        std::cout << "  Velocity: " << velocity << " rpm" << std::endl;
        std::cout << "  Temperature: " << static_cast<int>(temperature) << "°C" << std::endl;
        std::cout << "  Position (0-360°): " << static_cast<int>(position_u8) * 360 / 255 << "°" << std::endl;
        std::cout << "  Error Code: 0x" << std::hex << static_cast<int>(error_code) << std::endl;
    } else {
        std::cout << "CRC8ミスマッチ." << std::endl;
    }

    return 0;
}