#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <stdio.h>
#include <string.h>

#include "SerialPort.h"
#include "bitmap_image.h"

#include "DCSerial.h"
#include "DCImg.h"

using namespace std;

FILE* fp;
char *port_name = "\\\\.\\COM8";
char A2toPC2[1];
char PC2toA2[1];
char angle2[3] = {2, 0, 1};
char command_servo[] = {0x20, 0x24, 0x28};

bool sortcol( const vector<int>& v1,
              const vector<int>& v2 )
{
    return v1[1] < v2[1];
}

void call_from_thread()
{
    cout << "THARD2 : start thread cammmm" << endl;
    system("C: && cd C:\\\"Program Files (x86)\"\\Java\\jdk1.8.0_74\\bin && java code.SimpleRead");
    cout << "THARD2 : end thread cammmmm" << endl;
}

int main()
{
    FILE* fp;

    SerialPort arduino(port_name);
    if (arduino.isConnected())
    {
        cout << "PC2 : Connection Established" << endl;
        arduino.readSerialPort(A2toPC2, 1);
        printf("PC2 : %d, is mean create connection\n", *A2toPC2);
    }
    else
    {
        cout << "PC2 : ERROR, check port name" << endl;
    }

    //*A2toPC2 = 9;

    while (arduino.isConnected())
    {
        cout << "PC2 : Serial is connect" << endl;

        arduino.readSerialPort(A2toPC2, 1);
        printf("PC2 : %d", *A2toPC2);

        char control = getControl(*A2toPC2);
        char angle = getAngle(*A2toPC2);
        char SIZE = getSize(*A2toPC2) -1;
        printf("PC2 : Read Serial from A2, control : %d, angle: %d, size:%d\n", control, angle, SIZE);

        if (control == CONTROL_TREEPIC)
        {
            cout << "PC2 : START COMMAND 3PIC" << endl;

            vector< vector<int> > pre_size = {{0,0},{1,0}, {2,0}};
            for (int turn3time=0; turn3time<3; ++turn3time)
            {

                cout << "PC2 : round " << turn3time << endl;
                arduino.writeSerialPort(command_servo + turn3time, 1);
                char ack_servo[1];
                arduino.readSerialPort(ack_servo, 1);

                printf("PC2 : start new thread for run java\n");
                thread t1(call_from_thread);

                unsigned id=0;
                unsigned black;
                bool isNotOk = true;
                while (isNotOk)
                {
                    cout << "PC2 : round " << turn3time << ", pic_no ";
                    fp = waitNextBMP(fp, id);
                    printf(", Have new img id %d\n", id);
                    //unsigned black = countBlackWhite(fp, id);
                    black = countBlack(id);
                    printf("PC2 : round %d, id %u, number of black:%d\n", turn3time, id, black);

                    unsigned SIZE = separation(black);
                    if (SIZE == 5)           // 555 is very white non of above
                    {
                        printf("PC2 : round %d, !!!!!!!!!!!!!!!> id %u, number of black:%d, size:%d\n", turn3time, id, black, SIZE);
                    }
                    else
                    {
                        isNotOk = false;
                        printf("PC2 : round %d, ---------------> id %u, number of black:%d, SIZE%d\n", turn3time, id, black, SIZE);
                    }

                    ++id;
                }
                system("Taskkill /IM java.exe /F");
                t1.detach();
                cout << "THARD2 : kill thread for run java" << endl;

                system("C: && cd C:\\out && del *.bmp");
                cout << "PC2 : del *.bmp" << endl;

                pre_size[turn3time][0] = turn3time;
                pre_size[turn3time][1] = black;

                unsigned SIZE = separation(black);
                cout << "PC2 : -------------------> summary angle:" << turn3time << ", black is" << black << endl;
            }

            sort(pre_size.begin(), pre_size.end(),sortcol);
            cout << "PC2 : -------------------> summary angle:" << pre_size[0][0] << ", black is" << pre_size[0][1] << endl;
            cout << "PC2 : -------------------> summary angle:" << pre_size[1][0] << ", black is" << pre_size[1][1] << endl;
            cout << "PC2 : -------------------> summary angle:" << pre_size[2][0] << ", black is" <<pre_size[2][1] << endl;

            for (int i=0; i<3; ++i)
            {
                angle2[i] = pre_size[i][0];
                printf("@@@@@@@@@@@@@@@@@ %d -> %d\n", i, angle2[i]);
            }

            for (int i=0; i<3; ++i)
            {
                if (i==0)
                {
                    *PC2toA2 = 0x40 + 3;
                }
                else if (i==1)
                {
                    *PC2toA2 = 0x44 +4;
                }
                else if (i==2)
                {
                    *PC2toA2 = 0x48 +1;
                }
                cout << "PC2ToA2 1 : "  << *PC2toA2 << endl;
                arduino.writeSerialPort(PC2toA2, 1);
            }
        }
        else if (control == CONTROL_ONEPIC)
        {
            char comm_servoooo[1];
            printf("PC2 : ------------------- size: %d\n", SIZE);
            *comm_servoooo = command_servo[angle2[SIZE]];
            arduino.writeSerialPort(comm_servoooo, 1);
            char ack_servo[1];
            arduino.readSerialPort(ack_servo, 1);

            printf("PC2 : start new thread for run java\n");
            thread t1(call_from_thread);

            unsigned id=0;
            unsigned black;
            bool isNotOk = true;
            while (isNotOk)
            {
                fp = waitNextBMP(fp, id);
                printf(", Have new img id %d\n", id);
                //unsigned black = countBlackWhite(fp, id);
                black = countBlack(id);
                printf("PC2 : id %u, number of black:%d\n", id, black);

                unsigned SIZE = separation(black);
                if (SIZE == 5)           // 555 is very white non of above
                {
                    printf("PC2 : !!!!!!!!!!!!!!!> id %u, number of black:%d, size:%d\n", id, black, SIZE);
                }
                else
                {
                    isNotOk = false;
                    printf("PC2 : ---------------> id %u, number of black:%d, SIZE%d\n", id, black, SIZE);
                }

                ++id;
            }
            system("Taskkill /IM java.exe /F");
            t1.detach();
            cout << "THARD2 : kill thread for run java" << endl;

            system("C: && cd C:\\out && del *.bmp");
            cout << "PC2 : del *.bmp" << endl;

            unsigned SIZE = separation(black);
            cout << "PC2 : -------------------> size is" << SIZE << endl;

            *PC2toA2 = black/4800;
            cout << "PC2ToA2 Data : "  << *PC2toA2 << endl;
            *PC2toA2 += 0x50;
            cout << "PC2ToA2 2 : "  << (*PC2toA2)*4800 << endl;
            arduino.writeSerialPort(PC2toA2, 1);
        }
    }

    fclose(fp);
    return 0;
}
