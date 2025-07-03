#include <memory>
#include <algorithm>
#include <thread>
#include <cmath>
#include <vector>
#include <functional>
#include <stdexcept>
#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <boost/asio.hpp>

/*
    これはメカナムホイールを制御するROS2ノード
    cmd_vel topicを購読し、ロボットの移動を制御する

    -----cmd_vel topic-----
    msg.linear.x: 左右移動速度[m/s]
    msg.linear.y: 前後移動速度[m/s]
    msg.angular.z: 回転速度[rad/s]
    ------------------------

    コールバック関数cmd_vel_callbackでcmd_velトピックからのメッセージを受け取る
    void cmd_vel_callback(const geometry_msgs::msg::Twist::SharedPtr msg);
    この関数内でメカナムホイールの制御ロジックを実装する

    この関数内でmsgからvx, vy, wzの値を取得するには、
    ex.
    vx = msg->linear.x;

    このファイルの中身は自由に書き換えて良い。
    ただし、既に書かれている部分の変更は推奨しない。

    モータを制御するために送るバイト列はboost::asioを使用して送信すること。
    専用に新規にclassや関数を定義しても良い。
    ただし、このファイル内で完結するようにすること。

*/

using namespace boost::asio;
using namespace std;

class SerialDriver {
    public:
        SerialDriver(const string &port)
            : io(), serial(io, port), work(make_work_guard(io))
        {
            try {
                configureSerialPort();
                io_thread_ = thread([this]() { io.run(); });
            } catch (const boost::system::system_error &e) {
                throw runtime_error("Failed to configure serial port: " + string(e.what()));
            }
        }

        ~SerialDriver() {
            io.stop();
            if (io_thread_.joinable()) {
                io_thread_.join();
            }
        }

        void asyncSendData(const vector<uint8_t> &data, std::function<void(bool)> callback) {
            boost::asio::async_write(
                serial, boost::asio::buffer(data),
                [this, callback](const boost::system::error_code &ec, std::size_t /*bytes_transferred*/) {
                    if (ec) {
                        callback(false);
                        return;
                    }
                    callback(true);
                });
        }

        void sendMecanumCommand(/*args*/) {

            // メカナムホイールの制御コマンドを作成
            // ID付与やモードの設定は既に完了しているとする。
            // ここではvelocity loop modeでモータを制御するためのコマンドを作成する。
            // int型はuint8_tなどの明示的な型を使うことを推奨する。
            // わからないことは調べるか、質問すること

            // コマンドデータを格納するvector
            vector<uint8_t> command_data;

            /*
                Tips:

                vectorの末尾にバイトを追加するには、以下のように書く
                command_data.push_back(xxx); 
                ここでxxxは16進数の値で、1バイトのデータを表す
                16進数は0xFFのように表記する
                例：
                command_data.push_back(0x01);

                また、引数として受け取ったint16_t型の値を2バイトに分割して追加する場合は、以下のようにする
                high_byte = (value >> 8) & 0xFF; // 上位8ビット
                low_byte  = value & 0xFF;        // 下位8ビット
                注意点
                valueが負の値の場合を考慮する必要がある。

                acceleration timeはデフォルト値(0x00)を使用する

            */

            /*
                ここを実装
                関数を定義する場合はprivateに書くと良い
            */
            

            
            // チェックサム追加
            appendChecksum(command_data);
            
            // 非同期送信
            asyncSendData(command_data, [](bool success) {
                if (!success) {
                    // エラーハンドリング
                }
            });
        }

    private:
        // boost::asioで通信を行うためのメンバ変数
        io_context io;
        serial_port serial;
        thread io_thread_;
        executor_work_guard<io_context::executor_type> work;

        void configureSerialPort() {
            // /**/の部分をデータシートや仕様に基づいて適切な値に置き換えること
            // boost::asioのreferenceとddsm115のdocsを参照して記述すること。


            serial.set_option(serial_port_base::baud_rate(/**/));
            serial.set_option(serial_port_base::character_size(/**/));
            serial.set_option(serial_port_base::parity(/**/));
            serial.set_option(serial_port_base::stop_bits(/**/));
            serial.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));
        }

        // CRC8の計算
        // いじらないこと
        void appendChecksum(vector<uint8_t> &data) {
            uint8_t checksum = 0;
            for (auto byte : data) {
                checksum ^= byte;
            }
            data.push_back(checksum);
        }
};

class MecanumWheelControllerNode : public rclcpp::Node
{
    public:
        MecanumWheelControllerNode() : Node("mecanum_wheel_controller_node")
        {
            // topicの購読に必須
            cmd_vel_subscription_ = this->create_subscription<geometry_msgs::msg::Twist>(
            "cmd_vel", 10, std::bind(&MecanumWheelControllerNode::cmd_vel_callback, this, std::placeholders::_1));

        }
        

    private:
        // Subscriberの定義
        rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_subscription_;

        // SerialDriverをインスタンス化
        SerialDriver serial_driver_;

        // cmd_vel_topicのコールバック関数
        void cmd_vel_callback(const geometry_msgs::msg::Twist::SharedPtr msg) {
            /*
                ここがメインの実装
            */

            sendMecanumCommand(/*args*/);
        }
};


// ここは変更しない
int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MecanumWheelControllerNode>());
  rclcpp::shutdown();
  return 0;
}