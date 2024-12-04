#include "icb_gui.h"
#include <windows.h>

int FRM1;
int keypressed;
int boxX = 200, boxY = 380;      // Kýrmýzý kutunun baþlangýç koordinatlarý
int bulletX = -1, bulletY = -1;  // Mermi konumu
bool thread_continue = false;

#define TARGET_COUNT 10          // Hedef sayýsý
bool target_hit[TARGET_COUNT] = { false };  // Hedefler için vurulma durumu
int target_move_direction[TARGET_COUNT] = { 0 };  // Hedeflerin hareket yönü
int targetX[TARGET_COUNT];  // Hedeflerin yatay konumlarý
int targetY[TARGET_COUNT];  // Hedeflerin dikey konumlarý
ICBYTES m;

HANDLE HMutex;  // Mutex 
int MutexFlag = 0;  // Mutex flag

void ICGUI_Create() {
    ICG_MWTitle("GAME");
    ICG_MWSize(450, 500);
}

// Ateþ iþlevi
void Shoot() {
    if (bulletY == -1) {
        bulletX = boxX + 10;
        bulletY = boxY;
    }
}

// Ana kutu hareketi iþ parçacýðý
VOID* SlidingBox(PVOID lpParam) {
    DWORD dwWaitResult;
    while (1) {
        dwWaitResult = WaitForSingleObject(HMutex, INFINITE);
        MutexFlag = 1;

        FillRect(m, boxX, boxY, 20, 20, 0x000000);
        if (keypressed == 37) boxX -= 2;            // Sol
        if (keypressed == 39) boxX += 2;            // Sað
        FillRect(m, boxX, boxY, 20, 20, 0xff0000);  // Kýrmýzý kutuyu yeniden çiz
        DisplayImage(FRM1, m);

        ReleaseMutex(HMutex);
        MutexFlag = 0;
        Sleep(30);
    }
    return NULL;
}

// Mermi hareketi iþ parçacýðý
VOID* BulletMovement(PVOID lpParam) {
    DWORD dwWaitResult;
    while (1) {
        dwWaitResult = WaitForSingleObject(HMutex, INFINITE);
        MutexFlag = 1;

        if (bulletY != -1) {
            FillRect(m, bulletX, bulletY, 4, 8, 0x000000);
            bulletY -= 5;

            for (int i = 0; i < TARGET_COUNT; i++) {
                if (bulletY <= targetY[i] + 10 && bulletY >= targetY[i] && bulletX >= targetX[i] && bulletX <= targetX[i] + 30) {
                    int hitPos = bulletX - targetX[i];
                    if (hitPos >= 0 && hitPos < 9) target_move_direction[i] = 1;
                    else if (hitPos >= 9 && hitPos < 21) target_hit[i] = true;
                    else if (hitPos >= 21 && hitPos < 30) target_move_direction[i] = -1;

                    bulletY = -1;  // Mermiyi görünmez yap
                    break;
                }
            }

            if (bulletY < 0) bulletY = -1;
            if (bulletY != -1) FillRect(m, bulletX, bulletY, 4, 8, 0xffffff);  // Mermiyi yeniden çiz
            DisplayImage(FRM1, m);
        }

        ReleaseMutex(HMutex);
        MutexFlag = 0;
        Sleep(30);
    }
    return NULL;
}

// Hedef kutu hareketi iþ parçacýðý
VOID* TargetBox(PVOID lpParam) {
    DWORD dwWaitResult;
    while (1) {
        dwWaitResult = WaitForSingleObject(HMutex, INFINITE);
        MutexFlag = 1;

        for (int i = 0; i < TARGET_COUNT; i++) {
            FillRect(m, targetX[i], targetY[i], 30, 10, 0x000000);  // Kutuyu sil

            if (target_hit[i]) {
                // Eðer kutu vurulduysa, yeniden yukarýdan baþlat
                targetX[i] = rand() % 350 + 20;    // Rastgele yatay pozisyon
                targetY[i] = -rand() % 200;       // Yeniden baþlat
                target_hit[i] = false;
                target_move_direction[i] = 0;     // Hareket yönü sýfýrlanýr
            }

            // Eðer kutu hareket ediyorsa
            if (target_move_direction[i] != 0) {
                targetX[i] += target_move_direction[i] * 5;
                targetY[i] -= 5;
                if (targetY[i] <= -10) {
                    target_move_direction[i] = 0; // Hareket durdurulur
                }
            }
            else {
                // Normal düþüþ hareketi
                targetY[i] += 4;                 // Aþaðý doðru hareket
                if (targetY[i] > 400) {
                    targetX[i] = rand() % 350 + 20;  // Rastgele yeni yatay pozisyon
                    targetY[i] = -rand() % 200;      // Yukarýdan tekrar baþlat
                }
            }

            // Kutuyu yeniden çiz
            FillRect(m, targetX[i], targetY[i], 30, 10, 0x00ff00);
        }

        ReleaseMutex(HMutex);
        MutexFlag = 0;
        DisplayImage(FRM1, m);
        Sleep(50);  // Yenileme hýzý
    }
    return NULL;
}


// Baþlat/Durdur düðmesi iþlevi
void butonfonk() {
    DWORD dw;
    thread_continue = true;
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SlidingBox, NULL, 0, &dw);
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TargetBox, NULL, 0, &dw);
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BulletMovement, NULL, 0, &dw);
    SetFocus(ICG_GetMainWindow());
}

// Tuþ basýldýðýnda çaðrýlýr
void WhenKeyPressed(int k) {
    keypressed = k;
    if (k == 32) {  // Space tuþuna basýlýrsa ateþ et
        Shoot();
    }
}


void ICGUI_main() {
    ICG_Button(5, 5, 120, 25, "START / STOP", butonfonk);
    FRM1 = ICG_FrameMedium(5, 40, 400, 400);
    ICG_SetOnKeyPressed(WhenKeyPressed);
    CreateImage(m, 400, 400, ICB_UINT);

    // Hedeflerin baþlangýç konumlarýný ayarlama
    for (int i = 0; i < TARGET_COUNT; i++) {
        targetX[i] = rand() % 350 + 20;
        targetY[i] = rand() % 300 - 200;
    }

    HMutex = CreateMutex(NULL, FALSE, NULL);
}