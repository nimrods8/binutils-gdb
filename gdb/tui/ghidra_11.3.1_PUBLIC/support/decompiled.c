 
 ulong FUN_00112790(undefined1 *param_1,ulong param_2,char *param_3,int param_4,uint param_5,
 long param_6,char *param_7,char *param_8)
 
 {
 bool bVar1;
 bool bVar2;
 bool bVar3;
 bool bVar4;
 bool bVar5;
 bool bVar6;
 int iVar7;
 size_t sVar8;
 ushort **ppuVar9;
 long lVar10;
 char cVar11;
 byte *pbVar12;
 byte bVar13;
 ulong uVar14;
 ulong uVar15;
 ulong uVar16;
 uint uVar17;
 ulong uVar18;
 ulong uVar19;
 long in_FS_OFFSET;
 bool bVar20;
 bool bVar21;
 bool bVar22;
 bool bVar23;
 bool bVar24;
 bool bVar25;
 char *local_e0;
 char *local_d8;
 uint local_bc;
 ulong local_b8;
 char *local_b0;
 long local_80;
 ulong local_78;
 bool local_64;
 wint_t local_4c;
 mbstate_t local_48;
 long local_40;
 
 local_40 = *(long *)(in_FS_OFFSET + 0x28);
 local_e0 = param_7;
 uVar19 = 0xffffffffffffffff;
 local_d8 = param_8;
 local_bc = param_5;
 local_80 = param_6;
 LAB_001127f0:
 sVar8 = __ctype_get_mb_cur_max();
 uVar17 = local_bc & 2;
 bVar21 = uVar17 != 0;
 switch(param_4) {
 case 0:
 bVar6 = true;
 uVar15 = 0;
 local_64 = false;
 bVar2 = false;
 local_b8 = 0;
 bVar21 = false;
 bVar3 = false;
 local_78 = 0;
 local_b0 = (char *)0x0;
 break;
 case 2:
 bVar2 = false;
 bVar6 = true;
 bVar3 = false;
 local_64 = false;
 local_b8 = 0;
 goto LAB_00113933;
 case 3:
 bVar3 = true;
 LAB_0011305f:
 bVar6 = true;
 bVar21 = true;
 bVar2 = false;
 local_64 = false;
 local_b8 = 0;
 local_78 = 1;
 uVar15 = 0;
 param_4 = 2;
 local_b0 = "\'";
 break;
 case 4:
 if (uVar17 == 0) {
 local_64 = false;
 bVar6 = true;
 bVar2 = false;
 bVar3 = true;
 uVar18 = 0;
 uVar14 = param_2;
 goto LAB_00112f29;
 }
 case 1:
 bVar3 = false;
 goto LAB_0011305f;
 case 5:
 if (uVar17 == 0) {
 if (param_2 != 0) {
 *param_1 = 0x22;
 }
 bVar6 = true;
 uVar15 = 1;
 local_64 = false;
 bVar2 = false;
 local_b8 = 0;
 bVar21 = false;
 bVar3 = true;
 local_78 = 1;
 local_b0 = "\"";
 }
 else {
 bVar6 = true;
 uVar15 = 0;
 local_64 = false;
 bVar2 = false;
 local_b8 = 0;
 bVar21 = true;
 bVar3 = true;
 local_78 = 1;
 local_b0 = "\"";
 }
 break;
 case 6:
 bVar6 = true;
 uVar15 = 0;
 param_4 = 5;
 local_64 = false;
 bVar2 = false;
 local_b8 = 0;
 bVar21 = true;
 bVar3 = true;
 local_78 = 1;
 local_b0 = "\"";
 break;
 case 7:
 bVar6 = true;
 uVar15 = 0;
 local_64 = false;
 bVar2 = false;
 local_b8 = 0;
 bVar21 = false;
 bVar3 = true;
 local_78 = 0;
 local_b0 = (char *)0x0;
 break;
 case 8:
 case 9:
 case 10:
 if (param_4 != 10) {
 local_e0 = (char *)dcgettext(0,&DAT_0011b542,5);
 if (local_e0 == "`") {
 local_e0 = (char *)FUN_0010df40(&DAT_0011b542,param_4);
 }
 local_d8 = (char *)dcgettext(0,"\'",5);
 if (local_d8 == "\'") {
 local_d8 = (char *)FUN_0010df40("\'",param_4);
 }
 }
 uVar15 = 0;
 if (uVar17 == 0) {
 cVar11 = *local_e0;
 while (cVar11 != '\0') {
 if (uVar15 < param_2) {
 param_1[uVar15] = cVar11;
 }
 uVar15 = uVar15 + 1;
 cVar11 = local_e0[uVar15];
 }
 }
 local_78 = strlen(local_d8);
 local_b0 = local_d8;
 bVar6 = true;
 local_64 = false;
 bVar2 = false;
 local_b8 = 0;
 bVar3 = true;
 break;
 default:
 /* WARNING: Subroutine does not return */
 abort();
 }
 LAB_001128e8:
 uVar18 = 0;
 bVar1 = local_64;
 LAB_001128f0:
 bVar22 = uVar18 != uVar19;
 if (uVar19 == 0xffffffffffffffff) {
 bVar22 = param_3[uVar18] != '\0';
 }
 bVar24 = bVar3;
 if (!bVar22) goto LAB_00113706;
 bVar4 = (bool)(param_4 != 2 & bVar3);
 pbVar12 = (byte *)(param_3 + uVar18);
 bVar20 = (bool)(local_78 != 0 & bVar4);
 bVar25 = bVar2;
 bVar5 = bVar20;
 if (bVar20) {
 if ((uVar19 == 0xffffffffffffffff) && (1 < local_78)) {
 uVar19 = strlen(param_3);
 }
 if ((uVar19 < local_78 + uVar18) || (iVar7 = memcmp(pbVar12,local_b0,local_78), iVar7 != 0)) {
 bVar13 = *pbVar12;
 uVar14 = (ulong)bVar13;
 if ('?' < (char)bVar13) {
 if ('z' < (char)bVar13) {
 if (bVar13 == 0x7d) {
 bVar20 = false;
 }
 else {
 if ('}' < (char)bVar13) {
 bVar5 = false;
 if (bVar13 == 0x7e) goto LAB_00113a18;
 bVar20 = false;
 bVar13 = 0x7f;
 goto switchD_00112968_caseD_1;
 }
 bVar20 = false;
 bVar5 = false;
 if (bVar13 != 0x7b) {
 uVar14 = 0x7c;
 goto switchD_001131ac_caseD_21;
 }
 }
 goto LAB_00113504;
 }
 if (bVar13 != 0x40) {
 uVar16 = 1L << (bVar13 + 0xbf & 0x3f);
 if ((uVar16 & 0x3ffffff53ffffff) != 0) {
 bVar5 = false;
 goto LAB_00112999;
 }
 if ((uVar16 & 0xa4000000) == 0) goto LAB_0011355b;
 goto switchD_00113306_caseD_21;
 }
 switchD_00113306_caseD_1:
 bVar20 = false;
 goto switchD_00112968_caseD_1;
 }
 switch(uVar14) {
 case 0:
 goto switchD_00113306_caseD_0;
 default:
 goto switchD_00113306_caseD_1;
 case 7:
 bVar23 = false;
 bVar22 = false;
 bVar13 = 0x61;
 break;
 case 8:
 bVar23 = false;
 bVar22 = false;
 bVar13 = 0x62;
 break;
 case 9:
 bVar23 = false;
 bVar5 = false;
 uVar14 = 9;
 bVar13 = 0x74;
 goto joined_r0x00112c8e;
 case 10:
 bVar23 = false;
 bVar5 = false;
 uVar14 = 10;
 bVar13 = 0x6e;
 goto joined_r0x00112c8e;
 case 0xb:
 bVar23 = false;
 bVar22 = false;
 bVar13 = 0x76;
 break;
 case 0xc:
 bVar23 = false;
 bVar22 = false;
 bVar13 = 0x66;
 break;
 case 0xd:
 bVar23 = false;
 bVar5 = false;
 uVar14 = 0xd;
 bVar13 = 0x72;
 goto joined_r0x00112c8e;
 case 0x20:
 uVar14 = 0x20;
 bVar13 = 0x20;
 bVar5 = false;
 if ((bool)(bVar4 | bVar21)) goto LAB_00112999;
 bVar24 = false;
 goto LAB_00112a58;
 case 0x21:
 case 0x22:
 case 0x24:
 case 0x26:
 case 0x28:
 case 0x29:
 case 0x2a:
 case 0x3b:
 case 0x3c:
 case 0x3d:
 case 0x3e:
 switchD_00113306_caseD_21:
 bVar5 = false;
 goto switchD_001131ac_caseD_21;
 case 0x23:
 goto switchD_00112968_caseD_23;
 case 0x25:
 case 0x2b:
 case 0x2c:
 case 0x2d:
 case 0x2e:
 case 0x2f:
 case 0x30:
 case 0x31:
 case 0x32:
 case 0x33:
 case 0x34:
 case 0x35:
 case 0x36:
 case 0x37:
 case 0x38:
 case 0x39:
 case 0x3a:
 bVar5 = false;
 if ((bool)(bVar4 | bVar21)) goto LAB_00112999;
 bVar24 = false;
 goto LAB_00112a58;
 case 0x27:
 uVar14 = 0x27;
 bVar13 = 0x27;
 bVar5 = false;
 bVar1 = bVar20;
 if ((bool)(bVar4 | bVar21)) goto LAB_00112999;
 bVar24 = false;
 goto LAB_00112a58;
 case 0x3f:
 bVar5 = false;
 if (param_4 == 5) goto LAB_00112cd9;
 goto LAB_001133d0;
 }
 goto joined_r0x00112ca1;
 }
 if (!bVar21) {
 bVar13 = *pbVar12;
 uVar14 = (ulong)bVar13;
 if ('?' < (char)bVar13) {
 if ('z' < (char)bVar13) {
 if (bVar13 != 0x7d) {
 if ('}' < (char)bVar13) {
 if (bVar13 != 0x7e) goto LAB_00113590;
 LAB_00113a18:
 if (uVar18 == 0) {
 uVar14 = 0x7e;
 bVar13 = 0x7e;
 if (!(bool)(bVar4 | bVar21)) {
 bVar24 = false;
 goto LAB_00112a58;
 }
 goto LAB_00112999;
 }
 bVar20 = false;
 bVar22 = false;
 uVar14 = 0x7e;
 uVar16 = uVar15;
 goto LAB_0011299b;
 }
 if (bVar13 != 0x7b) {
 uVar14 = 0x7c;
 goto switchD_001131ac_caseD_21;
 }
 }
 goto LAB_00113504;
 }
 if (bVar13 == 0x40) goto switchD_00112968_caseD_1;
 uVar16 = 1L << (bVar13 + 0xbf & 0x3f);
 if ((uVar16 & 0x3ffffff53ffffff) != 0) goto LAB_00112999;
 if ((uVar16 & 0xa4000000) != 0) goto switchD_001131ac_caseD_21;
 LAB_001137fb:
 bVar5 = bVar20;
 if (!bVar3) goto switchD_001131ac_caseD_21;
 bVar22 = false;
 bVar13 = 0x5c;
 goto LAB_00112a0c;
 }
 switch(uVar14) {
 case 0:
 goto LAB_00113200;
 default:
 goto switchD_00112968_caseD_1;
 case 7:
 bVar23 = false;
 bVar22 = false;
 bVar13 = 0x61;
 break;
 case 8:
 bVar23 = false;
 bVar22 = false;
 bVar13 = 0x62;
 break;
 case 9:
 bVar23 = false;
 uVar14 = 9;
 bVar13 = 0x74;
 goto joined_r0x00112c8e;
 case 10:
 bVar23 = false;
 uVar14 = 10;
 bVar13 = 0x6e;
 goto joined_r0x00112c8e;
 case 0xb:
 bVar23 = false;
 bVar22 = false;
 bVar13 = 0x76;
 break;
 case 0xc:
 bVar23 = false;
 bVar22 = false;
 bVar13 = 0x66;
 break;
 case 0xd:
 bVar23 = false;
 uVar14 = 0xd;
 bVar13 = 0x72;
 goto joined_r0x00112c8e;
 case 0x20:
 uVar14 = 0x20;
 bVar13 = 0x20;
 if (!bVar4) {
 bVar24 = false;
 goto LAB_00112a58;
 }
 goto LAB_00112999;
 case 0x21:
 case 0x22:
 case 0x24:
 case 0x26:
 case 0x28:
 case 0x29:
 case 0x2a:
 case 0x3b:
 case 0x3c:
 case 0x3d:
 case 0x3e:
 goto switchD_001131ac_caseD_21;
 case 0x23:
 uVar14 = 0x23;
 goto LAB_00112e47;
 case 0x25:
 case 0x2b:
 case 0x2c:
 case 0x2d:
 case 0x2e:
 case 0x2f:
 case 0x30:
 case 0x31:
 case 0x32:
 case 0x33:
 case 0x34:
 case 0x35:
 case 0x36:
 case 0x37:
 case 0x38:
 case 0x39:
 case 0x3a:
 if (bVar4) goto LAB_00112999;
 bVar24 = false;
 goto LAB_00112a58;
 case 0x27:
 uVar14 = 0x27;
 bVar13 = 0x27;
 bVar1 = bVar20;
 if (bVar4) goto LAB_00112999;
 bVar24 = false;
 goto LAB_00112a58;
 case 0x3f:
 goto switchD_001131ac_caseD_3f;
 }
 goto LAB_001129d1;
 }
 goto LAB_00112e95;
 }
 bVar13 = *pbVar12;
 uVar14 = (ulong)bVar13;
 if ('?' < (char)bVar13) {
 if ('z' < (char)bVar13) {
 if (bVar13 != 0x7d) {
 if ((char)bVar13 < '~') {
 if (bVar13 != 0x7b) {
 bVar5 = false;
 uVar14 = 0x7c;
 bVar22 = false;
 goto LAB_00112c52;
 }
 goto LAB_00113504;
 }
 bVar5 = false;
 if (bVar13 == 0x7e) goto LAB_00112e47;
 LAB_00113590:
 bVar13 = 0x7f;
 goto joined_r0x00113598;
 }
 LAB_00113504:
 bVar5 = bVar20;
 if (uVar19 == 0xffffffffffffffff) {
 if (param_3[1] == '\0') goto LAB_00112e47;
 }
 else if (uVar19 == 1) {
 LAB_00112e47:
 bVar13 = (byte)uVar14;
 if (uVar18 != 0) {
 bVar20 = false;
 if ((bool)(bVar4 | bVar21)) goto LAB_00112999;
 bVar24 = false;
 goto LAB_00112a58;
 }
 goto LAB_00112c52;
 }
 goto switchD_001131ac_caseD_21;
 }
 if (bVar13 == 0x40) goto switchD_00112968_caseD_1;
 uVar16 = 1L << (bVar13 + 0xbf & 0x3f);
 if ((uVar16 & 0x3ffffff53ffffff) != 0) goto switchD_00112968_caseD_25;
 if ((uVar16 & 0xa4000000) != 0) goto switchD_00112968_caseD_21;
 if (param_4 == 2) {
 if (bVar21) goto LAB_00112e88;
 }
 else {
 LAB_0011355b:
 bVar20 = (bool)(bVar3 & bVar21);
 if (!bVar20) goto LAB_001137fb;
 if (local_78 == 0) goto LAB_00112e95;
 }
 uVar18 = uVar18 + 1;
 bVar22 = false;
 bVar13 = 0x5c;
 goto LAB_00112a72;
 }
 switch(uVar14) {
 case 0:
 if (bVar3) {
 switchD_00113306_caseD_0:
 if (!bVar21) {
 bVar24 = (bool)(param_4 == 2 & (bVar2 ^ 1U));
 bVar20 = bVar24;
 if (bVar24) {
 if (uVar15 < param_2) {
 param_1[uVar15] = 0x27;
 }
 if (uVar15 + 1 < param_2) {
 param_1[uVar15 + 1] = 0x24;
 }
 if (uVar15 + 2 < param_2) {
 param_1[uVar15 + 2] = 0x27;
 }
 uVar14 = uVar15 + 3;
 uVar15 = uVar15 + 4;
 bVar25 = bVar24;
 if (uVar14 < param_2) {
 param_1[uVar14] = 0x5c;
 bVar22 = false;
 bVar13 = 0x30;
 }
 else {
 bVar22 = false;
 bVar13 = 0x30;
 }
 goto LAB_00112a68;
 }
 LAB_00113200:
 uVar16 = uVar15 + 1;
 if (uVar15 < param_2) {
 param_1[uVar15] = 0x5c;
 }
 bVar5 = bVar20;
 if (!bVar4) {
 bVar13 = 0x30;
 uVar15 = uVar16;
 bVar24 = bVar22;
 bVar20 = false;
 goto LAB_00112a58;
 }
 if ((uVar18 + 1 < uVar19) && ((byte)(param_3[uVar18 + 1] - 0x30U) < 10)) {
 if (uVar16 < param_2) {
 param_1[uVar16] = 0x30;
 }
 if (uVar15 + 2 < param_2) {
 param_1[uVar15 + 2] = 0x30;
 }
 uVar16 = uVar15 + 3;
 }
 uVar14 = 0x30;
 bVar20 = false;
 goto LAB_0011299b;
 }
 if (param_4 == 2) {
 param_4 = 4;
 }
 goto LAB_00112e95;
 }
 if ((local_bc & 1) == 0) {
 bVar20 = false;
 uVar14 = 0;
 bVar13 = 0;
 bVar5 = false;
 if ((bool)(bVar4 | bVar21)) goto LAB_00112999;
 bVar24 = false;
 goto LAB_00112a58;
 }
 uVar18 = uVar18 + 1;
 goto LAB_001128f0;
 default:
 switchD_00112968_caseD_1:
 joined_r0x00113598:
 bVar23 = bVar3;
 if (sVar8 != 1) {
 local_48.__count = 0;
 local_48.__value = (_union_27)0x0;
 if (uVar19 == 0xffffffffffffffff) {
 uVar19 = strlen(param_3);
 }
 uVar16 = 0;
 do {
 uVar14 = uVar18 + uVar16;
 lVar10 = FUN_00111700(&local_4c,param_3 + uVar14,uVar19 - uVar14,&local_48);
 if (lVar10 == 0) break;
 if (lVar10 == -1) {
 bVar22 = false;
 goto LAB_001136f7;
 }
 if (lVar10 == -2) {
 if (uVar14 < uVar19) goto LAB_00113b7d;
 goto LAB_00113b83;
 }
 if (lVar10 != -3) {
 if ((param_4 == 2) && (bVar21)) {
 if (lVar10 == 1) {
 uVar16 = uVar16 + 1;
 goto LAB_00113698;
 }
 pbVar12 = (byte *)(param_3 + uVar14 + 1);
 do {
 if (((byte)(*pbVar12 - 0x5b) < 0x22) &&
 ((0x20000002bU >> ((ulong)(*pbVar12 - 0x5b) & 0x3f) & 1) != 0)) {
 param_4 = 2;
 goto LAB_00112e88;
 }
 pbVar12 = pbVar12 + 1;
 } while ((byte *)(param_3 + uVar14 + lVar10) != pbVar12);
 }
 uVar16 = uVar16 + lVar10;
 }
 LAB_00113698:
 iVar7 = iswprint(local_4c);
 if (iVar7 == 0) {
 bVar22 = false;
 }
 iVar7 = mbsinit(&local_48);
 } while (iVar7 == 0);
 bVar23 = (bool)((bVar22 ^ 1U) & bVar3);
 goto LAB_001136f7;
 }
 ppuVar9 = __ctype_b_loc();
 uVar16 = 1;
 bVar23 = (*(byte *)((long)*ppuVar9 + (ulong)bVar13 * 2 + 1) & 0x40) == 0;
 bVar22 = !bVar23;
 bVar23 = (bool)(bVar23 & bVar3);
 goto LAB_00112b05;
 case 7:
 bVar23 = param_4 == 2;
 uVar14 = 7;
 bVar13 = 0x61;
 break;
 case 8:
 bVar23 = param_4 == 2;
 uVar14 = 8;
 bVar13 = 0x62;
 break;
 case 9:
 uVar14 = 9;
 bVar13 = 0x74;
 goto LAB_00112e6d;
 case 10:
 uVar14 = 10;
 bVar13 = 0x6e;
 goto LAB_00112e6d;
 case 0xb:
 bVar23 = param_4 == 2;
 uVar14 = 0xb;
 bVar13 = 0x76;
 break;
 case 0xc:
 bVar23 = param_4 == 2;
 uVar14 = 0xc;
 bVar13 = 0x66;
 break;
 case 0xd:
 uVar14 = 0xd;
 bVar13 = 0x72;
 LAB_00112e6d:
 bVar23 = param_4 == 2;
 bVar5 = (bool)(bVar21 & bVar23);
 if ((bool)(bVar21 & bVar23)) goto LAB_00112e82;
 break;
 case 0x20:
 bVar5 = false;
 uVar14 = 0x20;
 goto LAB_00112c52;
 case 0x21:
 case 0x22:
 case 0x24:
 case 0x26:
 case 0x28:
 case 0x29:
 case 0x2a:
 case 0x3b:
 case 0x3c:
 case 0x3d:
 case 0x3e:
 switchD_00112968_caseD_21:
 bVar5 = false;
 bVar22 = false;
 LAB_00112c52:
 bVar13 = (byte)uVar14;
 if ((param_4 != 2) || (!bVar21)) {
 bVar20 = bVar22;
 if (!(bool)(bVar4 | bVar21)) {
 bVar24 = false;
 goto LAB_00112a58;
 }
 goto LAB_00112999;
 }
 goto LAB_00112e82;
 case 0x23:
 switchD_00112968_caseD_23:
 uVar14 = 0x23;
 bVar5 = false;
 goto LAB_00112e47;
 case 0x25:
 case 0x2b:
 case 0x2c:
 case 0x2d:
 case 0x2e:
 case 0x2f:
 case 0x30:
 case 0x31:
 case 0x32:
 case 0x33:
 case 0x34:
 case 0x35:
 case 0x36:
 case 0x37:
 case 0x38:
 case 0x39:
 case 0x3a:
 goto switchD_00112968_caseD_25;
 case 0x27:
 uVar14 = 0x27;
 bVar1 = bVar22;
 if (param_4 != 2) goto switchD_00112968_caseD_25;
 if (!bVar21) {
 if ((param_2 == 0) || (uVar14 = 0, uVar16 = param_2, local_b8 != 0)) {
 if (uVar15 < param_2) {
 param_1[uVar15] = 0x27;
 }
 if (uVar15 + 1 < param_2) {
 param_1[uVar15 + 1] = 0x5c;
 }
 uVar14 = param_2;
 uVar16 = local_b8;
 if (uVar15 + 2 < param_2) {
 param_1[uVar15 + 2] = 0x27;
 }
 }
 uVar15 = uVar15 + 3;
 bVar24 = false;
 bVar13 = 0x27;
 param_2 = uVar14;
 local_b8 = uVar16;
 bVar25 = false;
 goto LAB_00112a68;
 }
 goto LAB_00112e88;
 case 0x3f:
 if (param_4 == 2) {
 if (!bVar21) {
 bVar22 = false;
 bVar24 = false;
 bVar13 = 0x3f;
 goto LAB_00112a68;
 }
 goto LAB_00112e88;
 }
 switchD_001131ac_caseD_3f:
 if (param_4 != 5) {
 LAB_001133d0:
 bVar20 = false;
 uVar14 = 0x3f;
 bVar13 = 0x3f;
 if ((bool)(bVar4 | bVar21)) goto LAB_00112999;
 bVar24 = false;
 goto LAB_00112a58;
 }
 LAB_00112cd9:
 if ((local_bc & 4) == 0) goto LAB_001133d0;
 uVar16 = uVar18 + 2;
 bVar22 = false;
 uVar14 = 0x3f;
 if ((uVar19 <= uVar16) || (param_3[uVar18 + 1] != '?')) goto switchD_00112968_caseD_25;
 bVar13 = param_3[uVar16];
 uVar14 = (ulong)bVar13;
 bVar20 = bVar22;
 if ((bVar13 < 0x3f) && (bVar20 = (0x7000a38200000000U >> (uVar14 & 0x3f) & 1) != 0, bVar20)) {
 if (!bVar21) {
 if (uVar15 < param_2) {
 param_1[uVar15] = 0x3f;
 }
 if (uVar15 + 1 < param_2) {
 param_1[uVar15 + 1] = 0x22;
 }
 if (uVar15 + 2 < param_2) {
 param_1[uVar15 + 2] = 0x22;
 }
 if (uVar15 + 3 < param_2) {
 param_1[uVar15 + 3] = 0x3f;
 }
 uVar15 = uVar15 + 4;
 bVar20 = false;
 uVar18 = uVar16;
 if (bVar4) goto LAB_00112999;
 bVar24 = false;
 goto LAB_00112a58;
 }
 goto LAB_00112e95;
 }
 uVar14 = 0x3f;
 bVar13 = 0x3f;
 if ((bool)(bVar4 | bVar21)) goto LAB_00112999;
 bVar24 = false;
 goto LAB_00112a58;
 }
 joined_r0x00112c8e:
 if (!bVar3) {
 switchD_001131ac_caseD_21:
 bVar22 = false;
 goto switchD_00112968_caseD_25;
 }
 bVar22 = false;
 goto joined_r0x00112ca1;
 LAB_00113c66:
 param_4 = 5;
 param_2 = local_b8;
 goto LAB_001127f0;
 while( true ) {
 uVar16 = uVar16 + 1;
 uVar14 = uVar18 + uVar16;
 if (uVar19 <= uVar14) break;
 LAB_00113b7d:
 if (param_3[uVar14] == '\0') break;
 }
 LAB_00113b83:
 bVar22 = false;
 LAB_001136f7:
 if (1 < uVar16) {
 LAB_00112b14:
 uVar16 = uVar18 + uVar16;
 bVar24 = false;
 uVar14 = uVar18;
 do {
 if (bVar23) {
 bVar24 = param_4 == 2;
 if (bVar21) goto LAB_00112e88;
 bVar24 = (bool)(bVar24 & (bVar25 ^ 1U));
 if (bVar24) {
 if (uVar15 < param_2) {
 param_1[uVar15] = 0x27;
 }
 if (uVar15 + 1 < param_2) {
 param_1[uVar15 + 1] = 0x24;
 }
 if (uVar15 + 2 < param_2) {
 param_1[uVar15 + 2] = 0x27;
 }
 uVar15 = uVar15 + 3;
 bVar25 = bVar24;
 }
 if (uVar15 < param_2) {
 param_1[uVar15] = 0x5c;
 }
 if (uVar15 + 1 < param_2) {
 param_1[uVar15 + 1] = (bVar13 >> 6) + 0x30;
 }
 if (uVar15 + 2 < param_2) {
 param_1[uVar15 + 2] = (bVar13 >> 3 & 7) + 0x30;
 }
 uVar18 = uVar14 + 1;
 uVar15 = uVar15 + 3;
 bVar13 = (bVar13 & 7) + 0x30;
 bVar24 = bVar23;
 if (uVar16 <= uVar18) goto LAB_00112a1f;
 }
 else {
 bVar2 = (bool)((bVar24 ^ 1U) & bVar25);
 if (bVar20) {
 if (uVar15 < param_2) {
 param_1[uVar15] = 0x5c;
 }
 uVar15 = uVar15 + 1;
 }
 uVar18 = uVar14 + 1;
 if (uVar16 <= uVar18) goto LAB_00112a72;
 if (bVar2) {
 if (uVar15 < param_2) {
 param_1[uVar15] = 0x27;
 }
 if (uVar15 + 1 < param_2) {
 param_1[uVar15 + 1] = 0x27;
 }
 uVar15 = uVar15 + 2;
 bVar20 = false;
 bVar25 = false;
 }
 else {
 bVar20 = false;
 }
 }
 uVar14 = uVar14 + 1;
 if (uVar15 < param_2) {
 param_1[uVar15] = bVar13;
 }
 bVar13 = param_3[uVar14];
 uVar15 = uVar15 + 1;
 } while( true );
 }
 LAB_00112b05:
 uVar14 = (ulong)bVar13;
 bVar5 = bVar20;
 if (bVar23) {
 bVar22 = false;
 bVar23 = bVar3;
 goto LAB_00112b14;
 }
 switchD_00112968_caseD_25:
 bVar13 = (byte)uVar14;
 bVar24 = (bool)(bVar4 | bVar21);
 bVar20 = bVar22;
 if ((bool)(bVar4 | bVar21)) {
 LAB_00112999:
 bVar22 = false;
 uVar16 = uVar15;
 LAB_0011299b:
 bVar13 = (byte)uVar14;
 uVar15 = uVar16;
 bVar24 = bVar22;
 if ((local_80 == 0) || ((*(uint *)(local_80 + (uVar14 >> 5) * 4) >> (bVar13 & 0x1f) & 1) == 0))
 goto LAB_00112a58;
 bVar23 = param_4 == 2;
 bVar22 = bVar20;
 }
 else {
 LAB_00112a58:
 bVar22 = bVar20;
 bVar23 = param_4 == 2;
 if (!bVar5) {
 LAB_00112a68:
 uVar18 = uVar18 + 1;
 bVar2 = (bool)((bVar24 ^ 1U) & bVar25);
 LAB_00112a72:
 if (bVar2) {
 if (uVar15 < param_2) {
 param_1[uVar15] = 0x27;
 }
 if (uVar15 + 1 < param_2) {
 param_1[uVar15 + 1] = 0x27;
 }
 uVar15 = uVar15 + 2;
 bVar25 = false;
 }
 goto LAB_00112a1f;
 }
 }
 joined_r0x00112ca1:
 if (bVar21) {
 bVar24 = (bool)(bVar3 & bVar23);
 goto LAB_00112e88;
 }
 LAB_001129d1:
 bVar23 = (bool)((bVar2 ^ 1U) & bVar23);
 if (bVar23) {
 if (uVar15 < param_2) {
 param_1[uVar15] = 0x27;
 }
 if (uVar15 + 1 < param_2) {
 param_1[uVar15 + 1] = 0x24;
 }
 if (uVar15 + 2 < param_2) {
 param_1[uVar15 + 2] = 0x27;
 }
 uVar15 = uVar15 + 3;
 bVar2 = bVar23;
 }
 LAB_00112a0c:
 if (uVar15 < param_2) {
 param_1[uVar15] = 0x5c;
 }
 uVar15 = uVar15 + 1;
 uVar18 = uVar18 + 1;
 bVar25 = bVar2;
 LAB_00112a1f:
 bVar2 = bVar25;
 if (uVar15 < param_2) {
 param_1[uVar15] = bVar13;
 }
 uVar15 = uVar15 + 1;
 if (!bVar22) {
 bVar6 = false;
 }
 goto LAB_001128f0;
 LAB_00113706:
 bVar22 = param_4 == 2;
 bVar25 = uVar15 == 0;
 if (bVar25 && bVar22) {
 if (bVar21) {
 LAB_00112e82:
 param_4 = 2;
 LAB_00112e88:
 if (bVar24) {
 param_4 = 4;
 }
 LAB_00112e95:
 local_bc = local_bc & 0xfffffffd;
 local_80 = 0;
 goto LAB_001127f0;
 }
 if (bVar1) {
 if (bVar6) goto LAB_00113c66;
 local_64 = local_b8 != 0 && param_2 == 0;
 uVar18 = local_b8;
 uVar14 = local_b8;
 if (local_b8 != 0 && param_2 == 0) {
 LAB_00112f29:
 local_b8 = uVar14;
 if (local_b8 != 0) {
 *param_1 = 0x27;
 }
 uVar15 = 1;
 param_4 = 2;
 bVar21 = false;
 local_78 = 1;
 local_b0 = "\'";
 param_2 = local_b8;
 local_b8 = uVar18;
 goto LAB_001128e8;
 }
 uVar15 = 0;
 bVar21 = bVar1;
 }
 else {
 uVar15 = 0;
 bVar21 = bVar25 && bVar22;
 }
 }
 else {
 bVar21 = (bool)(bVar21 ^ 1);
 if (((bool)(bVar22 & bVar21)) && (bVar21 = (bool)(bVar22 & bVar21), bVar1)) {
 if (bVar6) goto LAB_00113c66;
 local_64 = param_2 == 0 && local_b8 != 0;
 bVar21 = bVar1;
 if (param_2 == 0 && local_b8 != 0) {
 param_2 = local_b8;
 bVar21 = bVar6;
 bVar6 = false;
 LAB_00113933:
 uVar15 = 0;
 local_78 = 1;
 local_b0 = "\'";
 param_4 = 2;
 uVar18 = local_b8;
 uVar14 = param_2;
 if (bVar21) goto LAB_001128e8;
 goto LAB_00112f29;
 }
 }
 }
 uVar19 = uVar15;
 if (((local_b0 != (char *)0x0) && (bVar21)) && (cVar11 = *local_b0, cVar11 != '\0')) {
 do {
 if (uVar19 < param_2) {
 param_1[uVar19] = cVar11;
 }
 uVar19 = uVar19 + 1;
 cVar11 = local_b0[uVar19 - uVar15];
 } while (cVar11 != '\0');
 }
 if (uVar19 < param_2) {
 param_1[uVar19] = 0;
 }
 if (local_40 != *(long *)(in_FS_OFFSET + 0x28)) {
 /* WARNING: Subroutine does not return */
 __stack_chk_fail();
 }
 return uVar19;
 }
 
