/*
サーボモーターが読み取るためにrpmに直して終了している
*/

#include <iostream>
#include <unistd.h>
#include <pigpiod_if2.h>
#include <vector>
#include <algorithm>
#include <cmath>

const std::vector<int> MOTOR_PINS = {17, 18, 27, 22}; // RF, LF, RB, LB

const int PULSE_POS_MAX  = 2000;
const int PULSE_STOP     = 1500;
const int PULSE_REV_MAX  = 1000;

// メカナムロボットの寸法!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
const double length = 0.2;   // [m]
const double wide = 0.2;   // [m]
const double radius = 0.03;  // ホイール半径[m]
const double MAX_WHEEL_RPM = 100.0; // サーボ最大回転数[rpm]（仮定）

class Mecanum {
    int pi;
    std::vector<int> pins;
public:
    Mecanum() : pi(pigpio_start(0, 0)), pins(MOTOR_PINS) {
        if (pi < 0) {
            std::cerr << "Failed to start pigpio" << std::endl;
            exit(1);
        }
        for (int pin : pins) {
            set_servo_pulsewidth(pi, pin, PULSE_STOP);
        }
    }
    ~Mecanum() {
        stop();
        pigpio_stop(pi);
    }
    

    void drive(double vx, double vy, double wz) {  // vx, vy: m/s, wz: rad/s

        // 各ホイールの角速度[rad/s]を計算
        double R = length + wide;
        double omega[4];
        omega[0] = (1.0/radius) * (vx - vy - R*wz); // FR
        omega[1] = (1.0/radius) * (vx + vy + R*wz); // FL
        omega[2] = (1.0/radius) * (vx + vy - R*wz); // BR
        omega[3] = (1.0/radius) * (vx - vy + R*wz); // BL

        // 最大回転数で正規化（仮にMAX_WHEEL_RPM=100rpm=10.47rad/s）
        double max_omega = 0.0;
        for (int i = 0; i < 4; ++i) max_omega = std::max(max_omega, std::abs(omega[i]));

        //最大回転数を超える場合は同じ比率だけすべての回転数を下げる
        if (max_omega > 2*M_PI*MAX_WHEEL_RPM/60.0) {
            double scale = (2*M_PI*MAX_WHEEL_RPM/60.0) / max_omega;
            for (int i = 0; i < 4; ++i) double motor_rpm = omega[i] * scale * 60.0 *(2*M_PI);//[rad/s] →　rpm[1/min]
        }

    }


    void stop() {
        for (int pin : pins) set_servo_pulsewidth(pi, pin, PULSE_STOP);
    }
};

int main() {
    Mecanum robot;
    double vx, vy, wz;
    std::cout << "vx[m/s] vy[m/s] wz[rad/s] をスペース区切りで入力: " << std::endl;
    while (std::cin >> vx >> vy >> wz) {
        robot.drive(vx, vy, wz);
        usleep(100000); // 0.1秒ごとに更新
        std::cout << "vx vy wz を入力: ";
    }
    robot.stop();
    return 0;
}