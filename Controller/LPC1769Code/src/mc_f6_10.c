/*****************************************************************************
 * $Id: mc_f6_10.c 6577 2011-06-06 12:00:00Z nxp16893 $
 *
 * Project: MC_lib
 *
 * Description: Motor Control fixed point libraries for F6_10
 *
 * Copyright(C) 2011, NXP Semiconductor
 * All rights reserved.
 *
 *****************************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
 *****************************************************************************/
#include "stdint.h" 
#include "mc_f6_10.h"
#include "mc_math.h"
#include "arm_math.h" 

// .full part of FIXED_6_10 fixed point variable with 804 steps from 0 to 0.5 PI rad
const int16_t F6_10_sin_lookup[805]=
{
	0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 
	58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106, 108, 
	110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 151, 
	153, 155, 157, 159, 161, 163, 165, 167, 169, 171, 173, 175, 177, 179, 181, 183, 185, 187, 189, 191, 193, 195, 
	197, 199, 201, 203, 205, 207, 209, 211, 213, 214, 216, 218, 220, 222, 224, 226, 228, 230, 232, 234, 236, 238, 
	240, 242, 244, 246, 248, 250, 251, 253, 255, 257, 259, 261, 263, 265, 267, 269, 271, 273, 275, 277, 279, 280, 
	282, 284, 286, 288, 290, 292, 294, 296, 298, 300, 302, 303, 305, 307, 309, 311, 313, 315, 317, 319, 321, 323, 
	324, 326, 328, 330, 332, 334, 336, 338, 340, 341, 343, 345, 347, 349, 351, 353, 355, 356, 358, 360, 362, 364, 
	366, 368, 370, 371, 373, 375, 377, 379, 381, 383, 384, 386, 388, 390, 392, 394, 396, 397, 399, 401, 403, 405, 
	407, 408, 410, 412, 414, 416, 418, 419, 421, 423, 425, 427, 429, 430, 432, 434, 436, 438, 439, 441, 443, 445, 
	447, 448, 450, 452, 454, 456, 457, 459, 461, 463, 465, 466, 468, 470, 472, 473, 475, 477, 479, 481, 482, 484, 
	486, 488, 489, 491, 493, 495, 496, 498, 500, 502, 503, 505, 507, 509, 510, 512, 514, 515, 517, 519, 521, 522, 
	524, 526, 528, 529, 531, 533, 534, 536, 538, 539, 541, 543, 545, 546, 548, 550, 551, 553, 555, 556, 558, 560, 
	561, 563, 565, 566, 568, 570, 571, 573, 575, 576, 578, 580, 581, 583, 585, 586, 588, 590, 591, 593, 594, 596, 
	598, 599, 601, 603, 604, 606, 607, 609, 611, 612, 614, 615, 617, 619, 620, 622, 623, 625, 627, 628, 630, 631, 
	633, 634, 636, 638, 639, 641, 642, 644, 645, 647, 648, 650, 652, 653, 655, 656, 658, 659, 661, 662, 664, 665, 
	667, 668, 670, 671, 673, 674, 676, 677, 679, 680, 682, 683, 685, 686, 688, 689, 691, 692, 694, 695, 697, 698, 
	700, 701, 703, 704, 705, 707, 708, 710, 711, 713, 714, 716, 717, 718, 720, 721, 723, 724, 725, 727, 728, 730, 
	731, 733, 734, 735, 737, 738, 739, 741, 742, 744, 745, 746, 748, 749, 750, 752, 753, 755, 756, 757, 759, 760, 
	761, 763, 764, 765, 767, 768, 769, 771, 772, 773, 774, 776, 777, 778, 780, 781, 782, 784, 785, 786, 787, 789, 
	790, 791, 793, 794, 795, 796, 798, 799, 800, 801, 803, 804, 805, 806, 807, 809, 810, 811, 812, 814, 815, 816, 
	817, 818, 820, 821, 822, 823, 824, 826, 827, 828, 829, 830, 831, 833, 834, 835, 836, 837, 838, 840, 841, 842, 
	843, 844, 845, 846, 848, 849, 850, 851, 852, 853, 854, 855, 856, 857, 859, 860, 861, 862, 863, 864, 865, 866, 
	867, 868, 869, 870, 871, 872, 874, 875, 876, 877, 878, 879, 880, 881, 882, 883, 884, 885, 886, 887, 888, 889, 
	890, 891, 892, 893, 894, 895, 896, 897, 898, 899, 900, 900, 901, 902, 903, 904, 905, 906, 907, 908, 909, 910, 
	911, 912, 913, 913, 914, 915, 916, 917, 918, 919, 920, 921, 921, 922, 923, 924, 925, 926, 927, 927, 928, 929, 
	930, 931, 932, 933, 933, 934, 935, 936, 937, 937, 938, 939, 940, 941, 941, 942, 943, 944, 945, 945, 946, 947, 
	948, 948, 949, 950, 951, 951, 952, 953, 954, 954, 955, 956, 956, 957, 958, 959, 959, 960, 961, 961, 962, 963, 
	963, 964, 965, 965, 966, 967, 967, 968, 969, 969, 970, 971, 971, 972, 973, 973, 974, 974, 975, 976, 976, 977, 
	977, 978, 979, 979, 980, 980, 981, 981, 982, 983, 983, 984, 984, 985, 985, 986, 986, 987, 988, 988, 989, 989, 
	990, 990, 991, 991, 992, 992, 993, 993, 994, 994, 995, 995, 996, 996, 996, 997, 997, 998, 998, 999, 999, 1000, 
	1000, 1000, 1001, 1001, 1002, 1002, 1003, 1003, 1003, 1004, 1004, 1005, 1005, 1005, 1006, 1006, 1006, 1007, 
	1007, 1008, 1008, 1008, 1009, 1009, 1009, 1010, 1010, 1010, 1011, 1011, 1011, 1012, 1012, 1012, 1012, 1013, 
	1013, 1013, 1014, 1014, 1014, 1014, 1015, 1015, 1015, 1015, 1016, 1016, 1016, 1016, 1017, 1017, 1017, 1017, 
	1018, 1018, 1018, 1018, 1019, 1019, 1019, 1019, 1019, 1020, 1020, 1020, 1020, 1020, 1020, 1021, 1021, 1021, 
	1021, 1021, 1021, 1021, 1022, 1022, 1022, 1022, 1022, 1022, 1022, 1022, 1023, 1023, 1023, 1023, 1023, 1023, 
	1023, 1023, 1023, 1023, 1023, 1023, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 
	1024, 1024, 1024, 1024 };	


/*****************************************************************************
** Function name:		F6_10_sin
**
** Description:			Sine calculation with lookuptable in F6_10 format
**
** Parameters:			None
** Returned value:		.full part of F6_10 fixed point variable 
**
*****************************************************************************/		
int16_t F6_10_sin(int16_t angle)
{
	uint16_t pos;

	if(angle < F6_10_CONST(0.0))angle += F6_10_CONST(PI*2);
   	pos = (uint16_t) angle;
	while(pos > F6_10_CONST(PI*0.5))pos -= F6_10_CONST(PI*0.5);
	// PI/2 = 1.5703125 = fix point full value 1608. 
	// Input range for 	F6_10_sin_lookup = 0 to 1608 FP full value. 
	// Divided by 2 = 804 steps	+ 1 = 805 steps
	pos >>= 1; // divide by 2
	if(angle <= F6_10_CONST(PI*0.5))									return F6_10_sin_lookup[pos];
	else if(angle>F6_10_CONST(PI*0.5) && angle <F6_10_CONST(PI))		return F6_10_sin_lookup[804-pos];
	else if(angle>=F6_10_CONST(PI) && angle < (F6_10_CONST(PI*1.5)))	return -F6_10_sin_lookup[pos];
	else 																return -F6_10_sin_lookup[804-pos];
}

/*****************************************************************************
** Function name:		F6_10_cos
**
** Description:			Cosine calculation with lookuptable in F6_10 format
**
** Parameters:			None
** Returned value:		.full part of F6_10 fixed point variable 
**
*****************************************************************************/		
int16_t F6_10_cos(int16_t angle)
{
	int16_t calc;
	if(angle < F6_10_CONST(0.0))angle += F6_10_CONST(PI*2);
	if(angle < F6_10_CONST(PI*1.5))calc = angle + F6_10_CONST(PI*0.5);
	else calc = angle - F6_10_CONST(PI*1.5);
	return F6_10_sin(calc);
}

/*****************************************************************************
**                            End Of File
******************************************************************************/
