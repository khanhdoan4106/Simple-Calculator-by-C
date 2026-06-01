#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void in_menu() {
    printf("\n=============================\n");
    printf("      MAY TINH CAM TAY       \n");
    printf("=============================\n");
    printf("  [1] Cong (+)\n");
    printf("  [2] Tru (-)\n");
    printf("  [3] Nhan (*)\n");
    printf("  [4] Chia (/)\n");
    printf("  [5] Chia lay du (%%)\n");
    printf("  [6] Luy thua (^)\n");
    printf("  [7] Can bac hai (sqrt)\n");
    printf("  [0] Thoat\n");
    printf("=============================\n");
}

double luy_thua(double co_so, int so_mu) {
    double ket_qua = 1.0;
    int am = 0;
    if (so_mu < 0) { am = 1; so_mu = -so_mu; }
    for (int i = 0; i < so_mu; i++) ket_qua *= co_so;
    return am ? 1.0 / ket_qua : ket_qua;
}

double can_bac_hai(double x) {
    if (x < 0) return -1;
    if (x == 0) return 0;
    double xn = x / 2.0;
    for (int i = 0; i < 1000; i++) {
        double xn1 = (xn + x / xn) / 2.0;
        if (xn1 - xn < 1e-10 && xn - xn1 < 1e-10) break;
        xn = xn1;
    }
    return xn;
}

int main() {
    int lua_chon;
    double a, b, ket_qua;
    char tiep_tuc;

    printf("\n  Chao mung den voi May Tinh Don Gian!\n");

    do {
        in_menu();
        printf("  Chon phep tinh: ");
        scanf("%d", &lua_chon);

        if (lua_chon == 0) {
            printf("\n  Tam biet! Hen gap lai.\n\n");
            break;
        }

        if (lua_chon < 1 || lua_chon > 7) {
            printf("  [!] Lua chon khong hop le. Vui long chon lai.\n");
            continue;
        }

        if (lua_chon == 7) {
            printf("  Nhap so can tinh can bac hai: ");
            scanf("%lf", &a);
            ket_qua = can_bac_hai(a);
            if (ket_qua < 0) {
                printf("  [!] Loi: Khong the tinh can bac hai cua so am!\n");
            } else {
                printf("  Ket qua: sqrt(%.4g) = %.6g\n", a, ket_qua);
            }
        } else {
            printf("  Nhap so thu nhat (a): ");
            scanf("%lf", &a);
            printf("  Nhap so thu hai  (b): ");
            scanf("%lf", &b);

            switch (lua_chon) {
                case 1:
                    ket_qua = a + b;
                    printf("  Ket qua: %.4g + %.4g = %.6g\n", a, b, ket_qua);
                    break;
                case 2:
                    ket_qua = a - b;
                    printf("  Ket qua: %.4g - %.4g = %.6g\n", a, b, ket_qua);
                    break;
                case 3:
                    ket_qua = a * b;
                    printf("  Ket qua: %.4g * %.4g = %.6g\n", a, b, ket_qua);
                    break;
                case 4:
                    if (b == 0) {
                        printf("  [!] Loi: Khong the chia cho 0!\n");
                    } else {
                        ket_qua = a / b;
                        printf("  Ket qua: %.4g / %.4g = %.6g\n", a, b, ket_qua);
                    }
                    break;
                case 5:
                    if ((int)b == 0) {
                        printf("  [!] Loi: Khong the chia lay du cho 0!\n");
                    } else {
                        int ia = (int)a, ib = (int)b;
                        printf("  Ket qua: %d %% %d = %d\n", ia, ib, ia % ib);
                    }
                    break;
                case 6: {
                    int so_mu = (int)b;
                    ket_qua = luy_thua(a, so_mu);
                    printf("  Ket qua: %.4g ^ %d = %.6g\n", a, so_mu, ket_qua);
                    break;
                }
            }
        }

        printf("\n  Tiep tuc tinh? (y/n): ");
        scanf(" %c", &tiep_tuc);

    } while (tiep_tuc == 'y' || tiep_tuc == 'Y');

    return 0;
}
