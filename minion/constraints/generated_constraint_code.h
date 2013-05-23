//  Call buildtree
//  Count:  100
//  Matched:  10
//  Matched:  20
//  Count:  200
//  Matched:  40
//  Tree cost: 166
//  Better tree found, of size:166
#ifdef PREPARE
#define SYMMETRIC
#else
int get_mapping_size() { return 16; }
vector<int> get_mapping_vector() {
  vector<int> v;
  v.push_back(0);   v.push_back(-1);
  v.push_back(0);   v.push_back(1);
  v.push_back(1);   v.push_back(-1);
  v.push_back(1);   v.push_back(1);
  v.push_back(2);   v.push_back(-1);
  v.push_back(2);   v.push_back(1);
  v.push_back(3);   v.push_back(-1);
  v.push_back(3);   v.push_back(1);
  v.push_back(4);   v.push_back(-1);
  v.push_back(4);   v.push_back(1);
  v.push_back(5);   v.push_back(-1);
  v.push_back(5);   v.push_back(1);
  v.push_back(6);   v.push_back(-3);
  v.push_back(6);   v.push_back(-1);
  v.push_back(6);   v.push_back(1);
  v.push_back(6);   v.push_back(3);
  return v; 
}
virtual void full_propagate() 
{
 FULL_PROPAGATE_INIT 
Label1:     (void)1;
PRINT_MACRO(1);
if(permutedInDomain(PERM_ARGS, 0,-1))
{ goto Label2; }
else
{ goto Label315; }
Label2:     (void)1;
PRINT_MACRO(2);
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label3; }
else
{ goto Label270; }
Label3:     (void)1;
PRINT_MACRO(3);
if(permutedInDomain(PERM_ARGS, 1,-1))
{ goto Label4; }
else
{ goto Label269; }
Label4:     (void)1;
PRINT_MACRO(4);
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label5; }
else
{ goto Label184; }
Label5:     (void)1;
PRINT_MACRO(5);
if(permutedInDomain(PERM_ARGS, 2,-1))
{ goto Label6; }
else
{ goto Label183; }
Label6:     (void)1;
PRINT_MACRO(6);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label7; }
else
{ goto Label156; }
Label7:     (void)1;
PRINT_MACRO(7);
if(permutedInDomain(PERM_ARGS, 3,-1))
{ goto Label8; }
else
{ goto Label155; }
Label8:     (void)1;
PRINT_MACRO(8);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label9; }
else
{ goto Label102; }
Label9:     (void)1;
PRINT_MACRO(9);
if(permutedInDomain(PERM_ARGS, 4,-1))
{ goto Label10; }
else
{ goto Label101; }
Label10:     (void)1;
PRINT_MACRO(10);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label11; }
else
{ goto Label72; }
Label11:     (void)1;
PRINT_MACRO(11);
if(permutedInDomain(PERM_ARGS, 5,-1))
{ goto Label12; }
else
{ goto Label71; }
Label12:     (void)1;
PRINT_MACRO(12);
if(permutedInDomain(PERM_ARGS, 5,1))
{ return; }
else
{ goto Label42; }
Label42:     (void)1;
PRINT_MACRO(42);
if(permutedInDomain(PERM_ARGS, 6,-3))
{ goto Label43; }
else
{ goto Label58; }
Label43:     (void)1;
PRINT_MACRO(43);
if(permutedInDomain(PERM_ARGS, 6,-1))
{ return; }
else
{ goto Label51; }
Label51:     (void)1;
PRINT_MACRO(51);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label55; }
Label55:     (void)1;
PRINT_MACRO(55);
if(permutedInDomain(PERM_ARGS, 6,3))
{ return; }
else
{ goto Label57; }
Label57:     (void)1;
PRINT_MACRO(57);
permutedRemoveFromDomain(PERM_ARGS, 4,-1);
return;
Label58:     (void)1;
PRINT_MACRO(58);
if(permutedInDomain(PERM_ARGS, 6,-1))
{ return; }
else
{ goto Label66; }
Label66:     (void)1;
PRINT_MACRO(66);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label70; }
Label70:     (void)1;
PRINT_MACRO(70);
permutedRemoveFromDomain(PERM_ARGS, 4,1);
return;
Label71:     (void)1;
PRINT_MACRO(71);
{
const int new_perm[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 9, 8, 11, 10, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label42;
Label72:     (void)1;
PRINT_MACRO(72);
if(permutedInDomain(PERM_ARGS, 5,-1))
{ goto Label73; }
else
{ goto Label88; }
Label73:     (void)1;
PRINT_MACRO(73);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label74; }
else
{ goto Label75; }
Label74:     (void)1;
PRINT_MACRO(74);
{
const int new_perm[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label42;
Label75:     (void)1;
PRINT_MACRO(75);
permutedRemoveFromDomain(PERM_ARGS, 6,-3);
return;
Label88:     (void)1;
PRINT_MACRO(88);
permutedRemoveFromDomain(PERM_ARGS, 6,3);
return;
Label101:     (void)1;
PRINT_MACRO(101);
{
const int new_perm[16] = { 0, 1, 3, 2, 5, 4, 6, 7, 9, 8, 10, 11, 15, 14, 13, 12, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label72;
Label102:     (void)1;
PRINT_MACRO(102);
if(permutedInDomain(PERM_ARGS, 4,-1))
{ goto Label103; }
else
{ goto Label154; }
Label103:     (void)1;
PRINT_MACRO(103);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label104; }
else
{ goto Label137; }
Label104:     (void)1;
PRINT_MACRO(104);
if(permutedInDomain(PERM_ARGS, 5,-1))
{ goto Label105; }
else
{ goto Label136; }
Label105:     (void)1;
PRINT_MACRO(105);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label106; }
else
{ goto Label107; }
Label106:     (void)1;
PRINT_MACRO(106);
{
const int new_perm[16] = { 0, 1, 2, 3, 8, 9, 10, 11, 4, 5, 6, 7, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label42;
Label107:     (void)1;
PRINT_MACRO(107);
if(permutedInDomain(PERM_ARGS, 6,-3))
{ goto Label108; }
else
{ goto Label123; }
Label108:     (void)1;
PRINT_MACRO(108);
if(permutedInDomain(PERM_ARGS, 6,-1))
{ return; }
else
{ goto Label116; }
Label116:     (void)1;
PRINT_MACRO(116);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label120; }
Label120:     (void)1;
PRINT_MACRO(120);
if(permutedInDomain(PERM_ARGS, 6,3))
{ return; }
else
{ goto Label122; }
Label122:     (void)1;
PRINT_MACRO(122);
permutedRemoveFromDomain(PERM_ARGS, 2,-1);
permutedRemoveFromDomain(PERM_ARGS, 4,-1);
return;
Label123:     (void)1;
PRINT_MACRO(123);
if(permutedInDomain(PERM_ARGS, 6,-1))
{ return; }
else
{ goto Label131; }
Label131:     (void)1;
PRINT_MACRO(131);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label135; }
Label135:     (void)1;
PRINT_MACRO(135);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
permutedRemoveFromDomain(PERM_ARGS, 4,1);
return;
Label136:     (void)1;
PRINT_MACRO(136);
{
const int new_perm[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 9, 8, 11, 10, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label107;
Label137:     (void)1;
PRINT_MACRO(137);
if(permutedInDomain(PERM_ARGS, 5,-1))
{ goto Label138; }
else
{ goto Label153; }
Label138:     (void)1;
PRINT_MACRO(138);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label139; }
else
{ goto Label140; }
Label139:     (void)1;
PRINT_MACRO(139);
{
const int new_perm[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label107;
Label140:     (void)1;
PRINT_MACRO(140);
permutedRemoveFromDomain(PERM_ARGS, 6,-3);
if(permutedInDomain(PERM_ARGS, 6,-1))
{ goto Label141; }
else
{ goto Label148; }
Label141:     (void)1;
PRINT_MACRO(141);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label145; }
Label145:     (void)1;
PRINT_MACRO(145);
if(permutedInDomain(PERM_ARGS, 6,3))
{ return; }
else
{ goto Label147; }
Label147:     (void)1;
PRINT_MACRO(147);
permutedRemoveFromDomain(PERM_ARGS, 2,-1);
return;
Label148:     (void)1;
PRINT_MACRO(148);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label152; }
Label152:     (void)1;
PRINT_MACRO(152);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
return;
Label153:     (void)1;
PRINT_MACRO(153);
permutedRemoveFromDomain(PERM_ARGS, 6,3);
{
const int new_perm[16] = { 0, 1, 3, 2, 5, 4, 6, 7, 8, 9, 11, 10, 15, 14, 13, 12, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label140;
Label154:     (void)1;
PRINT_MACRO(154);
{
const int new_perm[16] = { 0, 1, 3, 2, 5, 4, 6, 7, 9, 8, 10, 11, 15, 14, 13, 12, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label137;
Label155:     (void)1;
PRINT_MACRO(155);
{
const int new_perm[16] = { 0, 1, 2, 3, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label102;
Label156:     (void)1;
PRINT_MACRO(156);
if(permutedInDomain(PERM_ARGS, 3,-1))
{ goto Label157; }
else
{ goto Label182; }
Label157:     (void)1;
PRINT_MACRO(157);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label158; }
else
{ goto Label159; }
Label158:     (void)1;
PRINT_MACRO(158);
{
const int new_perm[16] = { 0, 1, 2, 3, 6, 7, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label102;
Label159:     (void)1;
PRINT_MACRO(159);
permutedRemoveFromDomain(PERM_ARGS, 6,-3);
if(permutedInDomain(PERM_ARGS, 4,-1))
{ goto Label160; }
else
{ goto Label181; }
Label160:     (void)1;
PRINT_MACRO(160);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label161; }
else
{ goto Label168; }
Label161:     (void)1;
PRINT_MACRO(161);
if(permutedInDomain(PERM_ARGS, 5,-1))
{ goto Label162; }
else
{ goto Label167; }
Label162:     (void)1;
PRINT_MACRO(162);
if(permutedInDomain(PERM_ARGS, 5,1))
{ return; }
else
{ goto Label166; }
Label166:     (void)1;
PRINT_MACRO(166);
{
const int new_perm[16] = { 0, 1, 2, 3, 8, 9, 10, 11, 4, 5, 6, 7, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label140;
Label167:     (void)1;
PRINT_MACRO(167);
{
const int new_perm[16] = { 0, 1, 2, 3, 9, 8, 11, 10, 4, 5, 6, 7, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label140;
Label168:     (void)1;
PRINT_MACRO(168);
if(permutedInDomain(PERM_ARGS, 5,-1))
{ goto Label169; }
else
{ goto Label176; }
Label169:     (void)1;
PRINT_MACRO(169);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label170; }
else
{ goto Label171; }
Label170:     (void)1;
PRINT_MACRO(170);
{
const int new_perm[16] = { 0, 1, 2, 3, 10, 11, 8, 9, 4, 5, 6, 7, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label140;
Label171:     (void)1;
PRINT_MACRO(171);
permutedRemoveFromDomain(PERM_ARGS, 6,-1);
return;
Label176:     (void)1;
PRINT_MACRO(176);
permutedRemoveFromDomain(PERM_ARGS, 6,3);
return;
Label181:     (void)1;
PRINT_MACRO(181);
{
const int new_perm[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 9, 8, 11, 10, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label168;
Label182:     (void)1;
PRINT_MACRO(182);
permutedRemoveFromDomain(PERM_ARGS, 6,3);
{
const int new_perm[16] = { 0, 1, 3, 2, 4, 5, 7, 6, 8, 9, 11, 10, 15, 14, 13, 12, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label159;
Label183:     (void)1;
PRINT_MACRO(183);
{
const int new_perm[16] = { 0, 1, 3, 2, 5, 4, 6, 7, 8, 9, 11, 10, 15, 14, 13, 12, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label156;
Label184:     (void)1;
PRINT_MACRO(184);
if(permutedInDomain(PERM_ARGS, 2,-1))
{ goto Label185; }
else
{ goto Label268; }
Label185:     (void)1;
PRINT_MACRO(185);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label186; }
else
{ goto Label243; }
Label186:     (void)1;
PRINT_MACRO(186);
if(permutedInDomain(PERM_ARGS, 3,-1))
{ goto Label187; }
else
{ goto Label242; }
Label187:     (void)1;
PRINT_MACRO(187);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label188; }
else
{ goto Label189; }
Label188:     (void)1;
PRINT_MACRO(188);
{
const int new_perm[16] = { 4, 5, 6, 7, 0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label102;
Label189:     (void)1;
PRINT_MACRO(189);
if(permutedInDomain(PERM_ARGS, 4,-1))
{ goto Label190; }
else
{ goto Label241; }
Label190:     (void)1;
PRINT_MACRO(190);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label191; }
else
{ goto Label224; }
Label191:     (void)1;
PRINT_MACRO(191);
if(permutedInDomain(PERM_ARGS, 5,-1))
{ goto Label192; }
else
{ goto Label223; }
Label192:     (void)1;
PRINT_MACRO(192);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label193; }
else
{ goto Label194; }
Label193:     (void)1;
PRINT_MACRO(193);
{
const int new_perm[16] = { 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label107;
Label194:     (void)1;
PRINT_MACRO(194);
if(permutedInDomain(PERM_ARGS, 6,-3))
{ goto Label195; }
else
{ goto Label210; }
Label195:     (void)1;
PRINT_MACRO(195);
if(permutedInDomain(PERM_ARGS, 6,-1))
{ return; }
else
{ goto Label203; }
Label203:     (void)1;
PRINT_MACRO(203);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label207; }
Label207:     (void)1;
PRINT_MACRO(207);
if(permutedInDomain(PERM_ARGS, 6,3))
{ return; }
else
{ goto Label209; }
Label209:     (void)1;
PRINT_MACRO(209);
permutedRemoveFromDomain(PERM_ARGS, 0,-1);
permutedRemoveFromDomain(PERM_ARGS, 2,-1);
permutedRemoveFromDomain(PERM_ARGS, 4,-1);
return;
Label210:     (void)1;
PRINT_MACRO(210);
if(permutedInDomain(PERM_ARGS, 6,-1))
{ return; }
else
{ goto Label218; }
Label218:     (void)1;
PRINT_MACRO(218);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label222; }
Label222:     (void)1;
PRINT_MACRO(222);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
permutedRemoveFromDomain(PERM_ARGS, 4,1);
return;
Label223:     (void)1;
PRINT_MACRO(223);
{
const int new_perm[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 9, 8, 11, 10, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label194;
Label224:     (void)1;
PRINT_MACRO(224);
if(permutedInDomain(PERM_ARGS, 5,-1))
{ goto Label225; }
else
{ goto Label240; }
Label225:     (void)1;
PRINT_MACRO(225);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label226; }
else
{ goto Label227; }
Label226:     (void)1;
PRINT_MACRO(226);
{
const int new_perm[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label194;
Label227:     (void)1;
PRINT_MACRO(227);
permutedRemoveFromDomain(PERM_ARGS, 6,-3);
if(permutedInDomain(PERM_ARGS, 6,-1))
{ goto Label228; }
else
{ goto Label235; }
Label228:     (void)1;
PRINT_MACRO(228);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label232; }
Label232:     (void)1;
PRINT_MACRO(232);
if(permutedInDomain(PERM_ARGS, 6,3))
{ return; }
else
{ goto Label234; }
Label234:     (void)1;
PRINT_MACRO(234);
permutedRemoveFromDomain(PERM_ARGS, 0,-1);
permutedRemoveFromDomain(PERM_ARGS, 2,-1);
return;
Label235:     (void)1;
PRINT_MACRO(235);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label239; }
Label239:     (void)1;
PRINT_MACRO(239);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
return;
Label240:     (void)1;
PRINT_MACRO(240);
permutedRemoveFromDomain(PERM_ARGS, 6,3);
{
const int new_perm[16] = { 1, 0, 2, 3, 5, 4, 6, 7, 8, 9, 11, 10, 15, 14, 13, 12, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label227;
Label241:     (void)1;
PRINT_MACRO(241);
{
const int new_perm[16] = { 1, 0, 2, 3, 5, 4, 6, 7, 9, 8, 10, 11, 15, 14, 13, 12, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label224;
Label242:     (void)1;
PRINT_MACRO(242);
{
const int new_perm[16] = { 0, 1, 2, 3, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label189;
Label243:     (void)1;
PRINT_MACRO(243);
if(permutedInDomain(PERM_ARGS, 3,-1))
{ goto Label244; }
else
{ goto Label267; }
Label244:     (void)1;
PRINT_MACRO(244);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label245; }
else
{ goto Label246; }
Label245:     (void)1;
PRINT_MACRO(245);
{
const int new_perm[16] = { 0, 1, 2, 3, 6, 7, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label189;
Label246:     (void)1;
PRINT_MACRO(246);
permutedRemoveFromDomain(PERM_ARGS, 6,-3);
if(permutedInDomain(PERM_ARGS, 4,-1))
{ goto Label247; }
else
{ goto Label266; }
Label247:     (void)1;
PRINT_MACRO(247);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label248; }
else
{ goto Label253; }
Label248:     (void)1;
PRINT_MACRO(248);
if(permutedInDomain(PERM_ARGS, 5,-1))
{ goto Label249; }
else
{ goto Label252; }
Label249:     (void)1;
PRINT_MACRO(249);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label250; }
else
{ goto Label251; }
Label250:     (void)1;
PRINT_MACRO(250);
{
const int new_perm[16] = { 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label140;
Label251:     (void)1;
PRINT_MACRO(251);
{
const int new_perm[16] = { 0, 1, 2, 3, 8, 9, 10, 11, 4, 5, 6, 7, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label227;
Label252:     (void)1;
PRINT_MACRO(252);
{
const int new_perm[16] = { 0, 1, 2, 3, 9, 8, 11, 10, 4, 5, 6, 7, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label227;
Label253:     (void)1;
PRINT_MACRO(253);
if(permutedInDomain(PERM_ARGS, 5,-1))
{ goto Label254; }
else
{ goto Label261; }
Label254:     (void)1;
PRINT_MACRO(254);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label255; }
else
{ goto Label256; }
Label255:     (void)1;
PRINT_MACRO(255);
{
const int new_perm[16] = { 0, 1, 2, 3, 10, 11, 8, 9, 4, 5, 6, 7, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label227;
Label256:     (void)1;
PRINT_MACRO(256);
permutedRemoveFromDomain(PERM_ARGS, 6,-1);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label257; }
else
{ goto Label260; }
Label257:     (void)1;
PRINT_MACRO(257);
if(permutedInDomain(PERM_ARGS, 6,3))
{ return; }
else
{ goto Label259; }
Label259:     (void)1;
PRINT_MACRO(259);
permutedRemoveFromDomain(PERM_ARGS, 0,-1);
return;
Label260:     (void)1;
PRINT_MACRO(260);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
return;
Label261:     (void)1;
PRINT_MACRO(261);
permutedRemoveFromDomain(PERM_ARGS, 6,3);
if(permutedInDomain(PERM_ARGS, 6,-1))
{ goto Label262; }
else
{ goto Label265; }
Label262:     (void)1;
PRINT_MACRO(262);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label264; }
Label264:     (void)1;
PRINT_MACRO(264);
permutedRemoveFromDomain(PERM_ARGS, 0,-1);
return;
Label265:     (void)1;
PRINT_MACRO(265);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
return;
Label266:     (void)1;
PRINT_MACRO(266);
{
const int new_perm[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 9, 8, 11, 10, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label253;
Label267:     (void)1;
PRINT_MACRO(267);
permutedRemoveFromDomain(PERM_ARGS, 6,3);
{
const int new_perm[16] = { 1, 0, 2, 3, 4, 5, 7, 6, 8, 9, 11, 10, 15, 14, 13, 12, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label246;
Label268:     (void)1;
PRINT_MACRO(268);
{
const int new_perm[16] = { 1, 0, 2, 3, 5, 4, 6, 7, 8, 9, 11, 10, 15, 14, 13, 12, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label243;
Label269:     (void)1;
PRINT_MACRO(269);
{
const int new_perm[16] = { 1, 0, 3, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label184;
Label270:     (void)1;
PRINT_MACRO(270);
if(permutedInDomain(PERM_ARGS, 1,-1))
{ goto Label271; }
else
{ goto Label314; }
Label271:     (void)1;
PRINT_MACRO(271);
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label272; }
else
{ goto Label273; }
Label272:     (void)1;
PRINT_MACRO(272);
{
const int new_perm[16] = { 2, 3, 0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label184;
Label273:     (void)1;
PRINT_MACRO(273);
permutedRemoveFromDomain(PERM_ARGS, 6,-3);
if(permutedInDomain(PERM_ARGS, 2,-1))
{ goto Label274; }
else
{ goto Label313; }
Label274:     (void)1;
PRINT_MACRO(274);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label275; }
else
{ goto Label280; }
Label275:     (void)1;
PRINT_MACRO(275);
if(permutedInDomain(PERM_ARGS, 3,-1))
{ goto Label276; }
else
{ goto Label279; }
Label276:     (void)1;
PRINT_MACRO(276);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label277; }
else
{ goto Label278; }
Label277:     (void)1;
PRINT_MACRO(277);
{
const int new_perm[16] = { 4, 5, 6, 7, 0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label159;
Label278:     (void)1;
PRINT_MACRO(278);
{
const int new_perm[16] = { 4, 5, 6, 7, 0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label246;
Label279:     (void)1;
PRINT_MACRO(279);
{
const int new_perm[16] = { 5, 4, 7, 6, 0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label246;
Label280:     (void)1;
PRINT_MACRO(280);
if(permutedInDomain(PERM_ARGS, 3,-1))
{ goto Label281; }
else
{ goto Label298; }
Label281:     (void)1;
PRINT_MACRO(281);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label282; }
else
{ goto Label283; }
Label282:     (void)1;
PRINT_MACRO(282);
{
const int new_perm[16] = { 6, 7, 4, 5, 0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label246;
Label283:     (void)1;
PRINT_MACRO(283);
permutedRemoveFromDomain(PERM_ARGS, 6,-1);
if(permutedInDomain(PERM_ARGS, 4,-1))
{ goto Label284; }
else
{ goto Label297; }
Label284:     (void)1;
PRINT_MACRO(284);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label285; }
else
{ goto Label292; }
Label285:     (void)1;
PRINT_MACRO(285);
if(permutedInDomain(PERM_ARGS, 5,-1))
{ goto Label286; }
else
{ goto Label291; }
Label286:     (void)1;
PRINT_MACRO(286);
if(permutedInDomain(PERM_ARGS, 5,1))
{ return; }
else
{ goto Label290; }
Label290:     (void)1;
PRINT_MACRO(290);
{
const int new_perm[16] = { 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label256;
Label291:     (void)1;
PRINT_MACRO(291);
{
const int new_perm[16] = { 9, 8, 11, 10, 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label256;
Label292:     (void)1;
PRINT_MACRO(292);
if(permutedInDomain(PERM_ARGS, 5,-1))
{ goto Label293; }
else
{ goto Label296; }
Label293:     (void)1;
PRINT_MACRO(293);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label294; }
else
{ goto Label295; }
Label294:     (void)1;
PRINT_MACRO(294);
{
const int new_perm[16] = { 10, 11, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label256;
Label295:     (void)1;
PRINT_MACRO(295);
permutedRemoveFromDomain(PERM_ARGS, 6,1);
return;
Label296:     (void)1;
PRINT_MACRO(296);
permutedRemoveFromDomain(PERM_ARGS, 6,3);
return;
Label297:     (void)1;
PRINT_MACRO(297);
{
const int new_perm[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 9, 8, 11, 10, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label292;
Label298:     (void)1;
PRINT_MACRO(298);
permutedRemoveFromDomain(PERM_ARGS, 6,3);
if(permutedInDomain(PERM_ARGS, 4,-1))
{ goto Label299; }
else
{ goto Label312; }
Label299:     (void)1;
PRINT_MACRO(299);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label300; }
else
{ goto Label307; }
Label300:     (void)1;
PRINT_MACRO(300);
if(permutedInDomain(PERM_ARGS, 5,-1))
{ goto Label301; }
else
{ goto Label306; }
Label301:     (void)1;
PRINT_MACRO(301);
if(permutedInDomain(PERM_ARGS, 5,1))
{ return; }
else
{ goto Label305; }
Label305:     (void)1;
PRINT_MACRO(305);
{
const int new_perm[16] = { 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label261;
Label306:     (void)1;
PRINT_MACRO(306);
{
const int new_perm[16] = { 9, 8, 11, 10, 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label261;
Label307:     (void)1;
PRINT_MACRO(307);
if(permutedInDomain(PERM_ARGS, 5,-1))
{ goto Label308; }
else
{ goto Label311; }
Label308:     (void)1;
PRINT_MACRO(308);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label309; }
else
{ goto Label310; }
Label309:     (void)1;
PRINT_MACRO(309);
{
const int new_perm[16] = { 10, 11, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label261;
Label310:     (void)1;
PRINT_MACRO(310);
permutedRemoveFromDomain(PERM_ARGS, 6,-1);
return;
Label311:     (void)1;
PRINT_MACRO(311);
permutedRemoveFromDomain(PERM_ARGS, 6,1);
return;
Label312:     (void)1;
PRINT_MACRO(312);
{
const int new_perm[16] = { 4, 5, 7, 6, 0, 1, 3, 2, 9, 8, 10, 11, 15, 14, 13, 12, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label307;
Label313:     (void)1;
PRINT_MACRO(313);
{
const int new_perm[16] = { 0, 1, 2, 3, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label280;
Label314:     (void)1;
PRINT_MACRO(314);
permutedRemoveFromDomain(PERM_ARGS, 6,3);
{
const int new_perm[16] = { 0, 1, 3, 2, 4, 5, 7, 6, 8, 9, 11, 10, 15, 14, 13, 12, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label273;
Label315:     (void)1;
PRINT_MACRO(315);
{
const int new_perm[16] = { 1, 0, 2, 3, 4, 5, 7, 6, 8, 9, 11, 10, 15, 14, 13, 12, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label270;
}
#endif

//  Depth: 17
//  Number of nodes: 166
//  Number of nodes explored by algorithm: 315
//Group Size: 768
