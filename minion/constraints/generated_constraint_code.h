//  Call buildtree
//  Count:  100
//  Matched:  10
//  Count:  200
//  Matched:  20
//  Matched:  40
//  Count:  400
//  Matched:  80
//  Matched:  160
//  Count:  800
//  Matched:  320
//  Tree cost: 651
//  Better tree found, of size:651
#ifdef PREPARE
#define SYMMETRIC
#else
int get_mapping_size() { return 20; }
vector<int> get_mapping_vector() {
  vector<int> v;
  v.push_back(0);   v.push_back(0);
  v.push_back(0);   v.push_back(1);
  v.push_back(1);   v.push_back(0);
  v.push_back(1);   v.push_back(1);
  v.push_back(2);   v.push_back(0);
  v.push_back(2);   v.push_back(1);
  v.push_back(3);   v.push_back(0);
  v.push_back(3);   v.push_back(1);
  v.push_back(4);   v.push_back(0);
  v.push_back(4);   v.push_back(1);
  v.push_back(5);   v.push_back(0);
  v.push_back(5);   v.push_back(1);
  v.push_back(6);   v.push_back(0);
  v.push_back(6);   v.push_back(1);
  v.push_back(7);   v.push_back(0);
  v.push_back(7);   v.push_back(1);
  v.push_back(8);   v.push_back(0);
  v.push_back(8);   v.push_back(1);
  v.push_back(9);   v.push_back(0);
  v.push_back(9);   v.push_back(1);
  return v; 
}
virtual void full_propagate() 
{
 FULL_PROPAGATE_INIT 
Label1:     (void)1;
PRINT_MACRO(1);
if(permutedInDomain(PERM_ARGS, 9,1))
{ goto Label2; }
else
{ goto Label795; }
Label2:     (void)1;
PRINT_MACRO(2);
if(permutedInDomain(PERM_ARGS, 0,0))
{ goto Label3; }
else
{ goto Label782; }
Label3:     (void)1;
PRINT_MACRO(3);
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label4; }
else
{ goto Label613; }
Label4:     (void)1;
PRINT_MACRO(4);
if(permutedInDomain(PERM_ARGS, 1,0))
{ goto Label5; }
else
{ goto Label600; }
Label5:     (void)1;
PRINT_MACRO(5);
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label6; }
else
{ goto Label451; }
Label6:     (void)1;
PRINT_MACRO(6);
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label7; }
else
{ goto Label438; }
Label7:     (void)1;
PRINT_MACRO(7);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label8; }
else
{ goto Label325; }
Label8:     (void)1;
PRINT_MACRO(8);
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label9; }
else
{ goto Label312; }
Label9:     (void)1;
PRINT_MACRO(9);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label10; }
else
{ goto Label231; }
Label10:     (void)1;
PRINT_MACRO(10);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label11; }
else
{ goto Label218; }
Label11:     (void)1;
PRINT_MACRO(11);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label12; }
else
{ goto Label153; }
Label12:     (void)1;
PRINT_MACRO(12);
if(permutedInDomain(PERM_ARGS, 5,0))
{ return; }
else
{ goto Label134; }
Label134:     (void)1;
PRINT_MACRO(134);
if(permutedInDomain(PERM_ARGS, 6,0))
{ return; }
else
{ goto Label138; }
Label138:     (void)1;
PRINT_MACRO(138);
if(permutedInDomain(PERM_ARGS, 7,0))
{ return; }
else
{ goto Label142; }
Label142:     (void)1;
PRINT_MACRO(142);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label143; }
else
{ goto Label150; }
Label143:     (void)1;
PRINT_MACRO(143);
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label144; }
else
{ goto Label147; }
Label144:     (void)1;
PRINT_MACRO(144);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label146; }
Label146:     (void)1;
PRINT_MACRO(146);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
permutedRemoveFromDomain(PERM_ARGS, 3,1);
permutedRemoveFromDomain(PERM_ARGS, 4,1);
return;
Label147:     (void)1;
PRINT_MACRO(147);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label149; }
Label149:     (void)1;
PRINT_MACRO(149);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
permutedRemoveFromDomain(PERM_ARGS, 3,1);
permutedRemoveFromDomain(PERM_ARGS, 4,1);
return;
Label150:     (void)1;
PRINT_MACRO(150);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label152; }
Label152:     (void)1;
PRINT_MACRO(152);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
permutedRemoveFromDomain(PERM_ARGS, 3,1);
permutedRemoveFromDomain(PERM_ARGS, 4,1);
return;
Label153:     (void)1;
PRINT_MACRO(153);
if(permutedInDomain(PERM_ARGS, 5,0))
{ return; }
else
{ goto Label199; }
Label199:     (void)1;
PRINT_MACRO(199);
if(permutedInDomain(PERM_ARGS, 6,0))
{ return; }
else
{ goto Label203; }
Label203:     (void)1;
PRINT_MACRO(203);
if(permutedInDomain(PERM_ARGS, 7,0))
{ return; }
else
{ goto Label207; }
Label207:     (void)1;
PRINT_MACRO(207);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label208; }
else
{ goto Label215; }
Label208:     (void)1;
PRINT_MACRO(208);
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label209; }
else
{ goto Label212; }
Label209:     (void)1;
PRINT_MACRO(209);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label211; }
Label211:     (void)1;
PRINT_MACRO(211);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
permutedRemoveFromDomain(PERM_ARGS, 3,1);
return;
Label212:     (void)1;
PRINT_MACRO(212);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label214; }
Label214:     (void)1;
PRINT_MACRO(214);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
permutedRemoveFromDomain(PERM_ARGS, 3,1);
return;
Label215:     (void)1;
PRINT_MACRO(215);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label217; }
Label217:     (void)1;
PRINT_MACRO(217);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
permutedRemoveFromDomain(PERM_ARGS, 3,1);
return;
Label218:     (void)1;
PRINT_MACRO(218);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label219; }
else
{ goto Label222; }
Label219:     (void)1;
PRINT_MACRO(219);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label220; }
else
{ goto Label221; }
Label220:     (void)1;
PRINT_MACRO(220);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label134;
return;
Label221:     (void)1;
PRINT_MACRO(221);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label199;
return;
Label222:     (void)1;
PRINT_MACRO(222);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label223; }
else
{ goto Label226; }
Label223:     (void)1;
PRINT_MACRO(223);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label224; }
else
{ goto Label225; }
Label224:     (void)1;
PRINT_MACRO(224);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label138;
return;
Label225:     (void)1;
PRINT_MACRO(225);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label203;
return;
Label226:     (void)1;
PRINT_MACRO(226);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label227; }
else
{ goto Label230; }
Label227:     (void)1;
PRINT_MACRO(227);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label228; }
else
{ goto Label229; }
Label228:     (void)1;
PRINT_MACRO(228);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 14, 15, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label142;
return;
Label229:     (void)1;
PRINT_MACRO(229);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 14, 15, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label207;
return;
Label230:     (void)1;
PRINT_MACRO(230);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label231:     (void)1;
PRINT_MACRO(231);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label232; }
else
{ goto Label299; }
Label232:     (void)1;
PRINT_MACRO(232);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label233; }
else
{ goto Label234; }
Label233:     (void)1;
PRINT_MACRO(233);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 8, 9, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label153;
return;
Label234:     (void)1;
PRINT_MACRO(234);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label235; }
else
{ goto Label280; }
Label235:     (void)1;
PRINT_MACRO(235);
if(permutedInDomain(PERM_ARGS, 5,1))
{ return; }
else
{ goto Label237; }
Label237:     (void)1;
PRINT_MACRO(237);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label238; }
else
{ return; }
Label238:     (void)1;
PRINT_MACRO(238);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label240; }
Label240:     (void)1;
PRINT_MACRO(240);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label241; }
else
{ return; }
Label241:     (void)1;
PRINT_MACRO(241);
if(permutedInDomain(PERM_ARGS, 7,1))
{ return; }
else
{ goto Label243; }
Label243:     (void)1;
PRINT_MACRO(243);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label244; }
else
{ return; }
Label244:     (void)1;
PRINT_MACRO(244);
if(permutedInDomain(PERM_ARGS, 8,1))
{ return; }
else
{ goto Label248; }
Label248:     (void)1;
PRINT_MACRO(248);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label250; }
Label250:     (void)1;
PRINT_MACRO(250);
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 1,0);
permutedRemoveFromDomain(PERM_ARGS, 2,0);
return;
Label280:     (void)1;
PRINT_MACRO(280);
if(permutedInDomain(PERM_ARGS, 6,0))
{ return; }
else
{ goto Label284; }
Label284:     (void)1;
PRINT_MACRO(284);
if(permutedInDomain(PERM_ARGS, 7,0))
{ return; }
else
{ goto Label288; }
Label288:     (void)1;
PRINT_MACRO(288);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label289; }
else
{ goto Label296; }
Label289:     (void)1;
PRINT_MACRO(289);
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label290; }
else
{ goto Label293; }
Label290:     (void)1;
PRINT_MACRO(290);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label292; }
Label292:     (void)1;
PRINT_MACRO(292);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
return;
Label293:     (void)1;
PRINT_MACRO(293);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label295; }
Label295:     (void)1;
PRINT_MACRO(295);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
return;
Label296:     (void)1;
PRINT_MACRO(296);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label298; }
Label298:     (void)1;
PRINT_MACRO(298);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
return;
Label299:     (void)1;
PRINT_MACRO(299);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label300; }
else
{ goto Label303; }
Label300:     (void)1;
PRINT_MACRO(300);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label301; }
else
{ goto Label302; }
Label301:     (void)1;
PRINT_MACRO(301);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 10, 11, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label199;
return;
Label302:     (void)1;
PRINT_MACRO(302);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label280;
return;
Label303:     (void)1;
PRINT_MACRO(303);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label304; }
else
{ goto Label307; }
Label304:     (void)1;
PRINT_MACRO(304);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label305; }
else
{ goto Label306; }
Label305:     (void)1;
PRINT_MACRO(305);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 12, 13, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label203;
return;
Label306:     (void)1;
PRINT_MACRO(306);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label284;
return;
Label307:     (void)1;
PRINT_MACRO(307);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label308; }
else
{ goto Label311; }
Label308:     (void)1;
PRINT_MACRO(308);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label309; }
else
{ goto Label310; }
Label309:     (void)1;
PRINT_MACRO(309);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 14, 15, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label207;
return;
Label310:     (void)1;
PRINT_MACRO(310);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 14, 15, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label288;
return;
Label311:     (void)1;
PRINT_MACRO(311);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label312:     (void)1;
PRINT_MACRO(312);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label313; }
else
{ goto Label316; }
Label313:     (void)1;
PRINT_MACRO(313);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label314; }
else
{ goto Label315; }
Label314:     (void)1;
PRINT_MACRO(314);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 8, 9, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label218;
return;
Label315:     (void)1;
PRINT_MACRO(315);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 8, 9, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label299;
return;
Label316:     (void)1;
PRINT_MACRO(316);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label317; }
else
{ goto Label320; }
Label317:     (void)1;
PRINT_MACRO(317);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label318; }
else
{ goto Label319; }
Label318:     (void)1;
PRINT_MACRO(318);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 10, 11, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label222;
return;
Label319:     (void)1;
PRINT_MACRO(319);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 10, 11, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label303;
return;
Label320:     (void)1;
PRINT_MACRO(320);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label321; }
else
{ goto Label324; }
Label321:     (void)1;
PRINT_MACRO(321);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label322; }
else
{ goto Label323; }
Label322:     (void)1;
PRINT_MACRO(322);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 12, 13, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label226;
return;
Label323:     (void)1;
PRINT_MACRO(323);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 12, 13, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label307;
return;
Label324:     (void)1;
PRINT_MACRO(324);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label325:     (void)1;
PRINT_MACRO(325);
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label326; }
else
{ goto Label425; }
Label326:     (void)1;
PRINT_MACRO(326);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label327; }
else
{ goto Label328; }
Label327:     (void)1;
PRINT_MACRO(327);
{
const int new_perm[20] = { 0, 1, 2, 3, 6, 7, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label231;
return;
Label328:     (void)1;
PRINT_MACRO(328);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label329; }
else
{ goto Label412; }
Label329:     (void)1;
PRINT_MACRO(329);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label330; }
else
{ goto Label331; }
Label330:     (void)1;
PRINT_MACRO(330);
{
const int new_perm[20] = { 0, 1, 2, 3, 8, 9, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label234;
return;
Label331:     (void)1;
PRINT_MACRO(331);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label332; }
else
{ goto Label389; }
Label332:     (void)1;
PRINT_MACRO(332);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label333; }
else
{ goto Label334; }
Label333:     (void)1;
PRINT_MACRO(333);
{
const int new_perm[20] = { 0, 1, 2, 3, 10, 11, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label237;
return;
Label334:     (void)1;
PRINT_MACRO(334);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label335; }
else
{ goto Label360; }
Label335:     (void)1;
PRINT_MACRO(335);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label336; }
else
{ goto Label337; }
Label336:     (void)1;
PRINT_MACRO(336);
{
const int new_perm[20] = { 0, 1, 2, 3, 12, 13, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label240;
return;
Label337:     (void)1;
PRINT_MACRO(337);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label338; }
else
{ goto Label349; }
Label338:     (void)1;
PRINT_MACRO(338);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label339; }
else
{ goto Label340; }
Label339:     (void)1;
PRINT_MACRO(339);
{
const int new_perm[20] = { 0, 1, 2, 3, 14, 15, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label243;
return;
Label340:     (void)1;
PRINT_MACRO(340);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label341; }
else
{ goto Label346; }
Label341:     (void)1;
PRINT_MACRO(341);
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label342; }
else
{ goto Label345; }
Label342:     (void)1;
PRINT_MACRO(342);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label344; }
Label344:     (void)1;
PRINT_MACRO(344);
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 1,0);
permutedRemoveFromDomain(PERM_ARGS, 8,0);
return;
Label345:     (void)1;
PRINT_MACRO(345);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label346:     (void)1;
PRINT_MACRO(346);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label348; }
Label348:     (void)1;
PRINT_MACRO(348);
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 1,0);
return;
Label349:     (void)1;
PRINT_MACRO(349);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label350; }
else
{ return; }
Label350:     (void)1;
PRINT_MACRO(350);
if(permutedInDomain(PERM_ARGS, 8,1))
{ return; }
else
{ goto Label354; }
Label354:     (void)1;
PRINT_MACRO(354);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label356; }
Label356:     (void)1;
PRINT_MACRO(356);
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 1,0);
return;
Label360:     (void)1;
PRINT_MACRO(360);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label374; }
Label374:     (void)1;
PRINT_MACRO(374);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label375; }
else
{ return; }
Label375:     (void)1;
PRINT_MACRO(375);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label376; }
else
{ return; }
Label376:     (void)1;
PRINT_MACRO(376);
if(permutedInDomain(PERM_ARGS, 7,1))
{ return; }
else
{ goto Label378; }
Label378:     (void)1;
PRINT_MACRO(378);
if(permutedInDomain(PERM_ARGS, 8,1))
{ return; }
else
{ goto Label380; }
Label380:     (void)1;
PRINT_MACRO(380);
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 1,0);
return;
Label389:     (void)1;
PRINT_MACRO(389);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label390; }
else
{ goto Label393; }
Label390:     (void)1;
PRINT_MACRO(390);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label392; }
Label392:     (void)1;
PRINT_MACRO(392);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label360;
return;
Label393:     (void)1;
PRINT_MACRO(393);
if(permutedInDomain(PERM_ARGS, 7,0))
{ return; }
else
{ goto Label401; }
Label401:     (void)1;
PRINT_MACRO(401);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label402; }
else
{ goto Label409; }
Label402:     (void)1;
PRINT_MACRO(402);
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label403; }
else
{ goto Label406; }
Label403:     (void)1;
PRINT_MACRO(403);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label405; }
Label405:     (void)1;
PRINT_MACRO(405);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
return;
Label406:     (void)1;
PRINT_MACRO(406);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label408; }
Label408:     (void)1;
PRINT_MACRO(408);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
return;
Label409:     (void)1;
PRINT_MACRO(409);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label411; }
Label411:     (void)1;
PRINT_MACRO(411);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
return;
Label412:     (void)1;
PRINT_MACRO(412);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label413; }
else
{ goto Label416; }
Label413:     (void)1;
PRINT_MACRO(413);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label414; }
else
{ goto Label415; }
Label414:     (void)1;
PRINT_MACRO(414);
{
const int new_perm[20] = { 0, 1, 2, 3, 10, 11, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label280;
return;
Label415:     (void)1;
PRINT_MACRO(415);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label389;
return;
Label416:     (void)1;
PRINT_MACRO(416);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label417; }
else
{ goto Label420; }
Label417:     (void)1;
PRINT_MACRO(417);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label418; }
else
{ goto Label419; }
Label418:     (void)1;
PRINT_MACRO(418);
{
const int new_perm[20] = { 0, 1, 2, 3, 12, 13, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label284;
return;
Label419:     (void)1;
PRINT_MACRO(419);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label393;
return;
Label420:     (void)1;
PRINT_MACRO(420);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label421; }
else
{ goto Label424; }
Label421:     (void)1;
PRINT_MACRO(421);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label422; }
else
{ goto Label423; }
Label422:     (void)1;
PRINT_MACRO(422);
{
const int new_perm[20] = { 0, 1, 2, 3, 14, 15, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label288;
return;
Label423:     (void)1;
PRINT_MACRO(423);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 14, 15, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label401;
return;
Label424:     (void)1;
PRINT_MACRO(424);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label425:     (void)1;
PRINT_MACRO(425);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label426; }
else
{ goto Label429; }
Label426:     (void)1;
PRINT_MACRO(426);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label427; }
else
{ goto Label428; }
Label427:     (void)1;
PRINT_MACRO(427);
{
const int new_perm[20] = { 0, 1, 2, 3, 8, 9, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label299;
return;
Label428:     (void)1;
PRINT_MACRO(428);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 8, 9, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label412;
return;
Label429:     (void)1;
PRINT_MACRO(429);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label430; }
else
{ goto Label433; }
Label430:     (void)1;
PRINT_MACRO(430);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label431; }
else
{ goto Label432; }
Label431:     (void)1;
PRINT_MACRO(431);
{
const int new_perm[20] = { 0, 1, 2, 3, 10, 11, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label303;
return;
Label432:     (void)1;
PRINT_MACRO(432);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 10, 11, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label416;
return;
Label433:     (void)1;
PRINT_MACRO(433);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label434; }
else
{ goto Label437; }
Label434:     (void)1;
PRINT_MACRO(434);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label435; }
else
{ goto Label436; }
Label435:     (void)1;
PRINT_MACRO(435);
{
const int new_perm[20] = { 0, 1, 2, 3, 12, 13, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label307;
return;
Label436:     (void)1;
PRINT_MACRO(436);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 12, 13, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label420;
return;
Label437:     (void)1;
PRINT_MACRO(437);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label438:     (void)1;
PRINT_MACRO(438);
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label439; }
else
{ goto Label442; }
Label439:     (void)1;
PRINT_MACRO(439);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label440; }
else
{ goto Label441; }
Label440:     (void)1;
PRINT_MACRO(440);
{
const int new_perm[20] = { 0, 1, 2, 3, 6, 7, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label312;
return;
Label441:     (void)1;
PRINT_MACRO(441);
{
const int new_perm[20] = { 0, 1, 2, 3, 6, 7, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label425;
return;
Label442:     (void)1;
PRINT_MACRO(442);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label443; }
else
{ goto Label446; }
Label443:     (void)1;
PRINT_MACRO(443);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label444; }
else
{ goto Label445; }
Label444:     (void)1;
PRINT_MACRO(444);
{
const int new_perm[20] = { 0, 1, 2, 3, 8, 9, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label316;
return;
Label445:     (void)1;
PRINT_MACRO(445);
{
const int new_perm[20] = { 0, 1, 2, 3, 8, 9, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label429;
return;
Label446:     (void)1;
PRINT_MACRO(446);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label447; }
else
{ goto Label450; }
Label447:     (void)1;
PRINT_MACRO(447);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label448; }
else
{ goto Label449; }
Label448:     (void)1;
PRINT_MACRO(448);
{
const int new_perm[20] = { 0, 1, 2, 3, 10, 11, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label320;
return;
Label449:     (void)1;
PRINT_MACRO(449);
{
const int new_perm[20] = { 0, 1, 2, 3, 10, 11, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label433;
return;
Label450:     (void)1;
PRINT_MACRO(450);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label451:     (void)1;
PRINT_MACRO(451);
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label452; }
else
{ goto Label587; }
Label452:     (void)1;
PRINT_MACRO(452);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label453; }
else
{ goto Label454; }
Label453:     (void)1;
PRINT_MACRO(453);
{
const int new_perm[20] = { 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label325;
return;
Label454:     (void)1;
PRINT_MACRO(454);
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label455; }
else
{ goto Label574; }
Label455:     (void)1;
PRINT_MACRO(455);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label456; }
else
{ goto Label457; }
Label456:     (void)1;
PRINT_MACRO(456);
{
const int new_perm[20] = { 0, 1, 6, 7, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label328;
return;
Label457:     (void)1;
PRINT_MACRO(457);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label458; }
else
{ goto Label545; }
Label458:     (void)1;
PRINT_MACRO(458);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label459; }
else
{ goto Label460; }
Label459:     (void)1;
PRINT_MACRO(459);
{
const int new_perm[20] = { 0, 1, 8, 9, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label331;
return;
Label460:     (void)1;
PRINT_MACRO(460);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label461; }
else
{ goto Label494; }
Label461:     (void)1;
PRINT_MACRO(461);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label462; }
else
{ goto Label463; }
Label462:     (void)1;
PRINT_MACRO(462);
{
const int new_perm[20] = { 0, 1, 10, 11, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label334;
return;
Label463:     (void)1;
PRINT_MACRO(463);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label464; }
else
{ goto Label479; }
Label464:     (void)1;
PRINT_MACRO(464);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label465; }
else
{ goto Label466; }
Label465:     (void)1;
PRINT_MACRO(465);
{
const int new_perm[20] = { 0, 1, 12, 13, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label337;
return;
Label466:     (void)1;
PRINT_MACRO(466);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label467; }
else
{ goto Label470; }
Label467:     (void)1;
PRINT_MACRO(467);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label468; }
else
{ goto Label469; }
Label468:     (void)1;
PRINT_MACRO(468);
{
const int new_perm[20] = { 0, 1, 14, 15, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label340;
return;
Label469:     (void)1;
PRINT_MACRO(469);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label470:     (void)1;
PRINT_MACRO(470);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label471; }
else
{ goto Label476; }
Label471:     (void)1;
PRINT_MACRO(471);
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label472; }
else
{ goto Label475; }
Label472:     (void)1;
PRINT_MACRO(472);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label474; }
Label474:     (void)1;
PRINT_MACRO(474);
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 8,0);
return;
Label475:     (void)1;
PRINT_MACRO(475);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label476:     (void)1;
PRINT_MACRO(476);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label478; }
Label478:     (void)1;
PRINT_MACRO(478);
permutedRemoveFromDomain(PERM_ARGS, 0,0);
return;
Label479:     (void)1;
PRINT_MACRO(479);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label480; }
else
{ goto Label483; }
Label480:     (void)1;
PRINT_MACRO(480);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label481; }
else
{ goto Label482; }
Label481:     (void)1;
PRINT_MACRO(481);
{
const int new_perm[20] = { 0, 1, 14, 15, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label349;
return;
Label482:     (void)1;
PRINT_MACRO(482);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label470;
return;
Label483:     (void)1;
PRINT_MACRO(483);
if(permutedInDomain(PERM_ARGS, 9,0))
{ goto Label484; }
else
{ goto Label489; }
Label484:     (void)1;
PRINT_MACRO(484);
if(permutedInDomain(PERM_ARGS, 8,0))
{ return; }
else
{ goto Label488; }
Label488:     (void)1;
PRINT_MACRO(488);
permutedRemoveFromDomain(PERM_ARGS, 9,0);
return;
Label489:     (void)1;
PRINT_MACRO(489);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label490; }
else
{ return; }
Label490:     (void)1;
PRINT_MACRO(490);
if(permutedInDomain(PERM_ARGS, 8,1))
{ return; }
else
{ goto Label492; }
Label492:     (void)1;
PRINT_MACRO(492);
permutedRemoveFromDomain(PERM_ARGS, 0,0);
return;
Label494:     (void)1;
PRINT_MACRO(494);
if(permutedInDomain(PERM_ARGS, 9,0))
{ goto Label495; }
else
{ goto Label516; }
Label495:     (void)1;
PRINT_MACRO(495);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label496; }
else
{ goto Label507; }
Label496:     (void)1;
PRINT_MACRO(496);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label498; }
Label498:     (void)1;
PRINT_MACRO(498);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label499; }
else
{ goto Label506; }
Label499:     (void)1;
PRINT_MACRO(499);
if(permutedInDomain(PERM_ARGS, 7,1))
{ return; }
else
{ goto Label501; }
Label501:     (void)1;
PRINT_MACRO(501);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label502; }
else
{ return; }
Label502:     (void)1;
PRINT_MACRO(502);
if(permutedInDomain(PERM_ARGS, 8,1))
{ return; }
else
{ goto Label504; }
Label504:     (void)1;
PRINT_MACRO(504);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label506:     (void)1;
PRINT_MACRO(506);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label484;
return;
Label507:     (void)1;
PRINT_MACRO(507);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label508; }
else
{ return; }
Label508:     (void)1;
PRINT_MACRO(508);
if(permutedInDomain(PERM_ARGS, 7,1))
{ return; }
else
{ goto Label510; }
Label510:     (void)1;
PRINT_MACRO(510);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 14, 15, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label484;
return;
Label516:     (void)1;
PRINT_MACRO(516);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label517; }
else
{ goto Label532; }
Label517:     (void)1;
PRINT_MACRO(517);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label518; }
else
{ goto Label527; }
Label518:     (void)1;
PRINT_MACRO(518);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label519; }
else
{ goto Label524; }
Label519:     (void)1;
PRINT_MACRO(519);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label520; }
else
{ goto Label521; }
Label520:     (void)1;
PRINT_MACRO(520);
{
const int new_perm[20] = { 0, 1, 12, 13, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label376;
return;
Label521:     (void)1;
PRINT_MACRO(521);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label522; }
else
{ goto Label523; }
Label522:     (void)1;
PRINT_MACRO(522);
{
const int new_perm[20] = { 0, 1, 14, 15, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label378;
return;
Label523:     (void)1;
PRINT_MACRO(523);
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 8,0);
return;
Label524:     (void)1;
PRINT_MACRO(524);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label526; }
Label526:     (void)1;
PRINT_MACRO(526);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label490;
return;
Label527:     (void)1;
PRINT_MACRO(527);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label528; }
else
{ goto Label531; }
Label528:     (void)1;
PRINT_MACRO(528);
if(permutedInDomain(PERM_ARGS, 7,0))
{ return; }
else
{ goto Label530; }
Label530:     (void)1;
PRINT_MACRO(530);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
return;
Label531:     (void)1;
PRINT_MACRO(531);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 14, 15, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label490;
return;
Label532:     (void)1;
PRINT_MACRO(532);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label533; }
else
{ goto Label540; }
Label533:     (void)1;
PRINT_MACRO(533);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label535; }
Label535:     (void)1;
PRINT_MACRO(535);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label536; }
else
{ return; }
Label536:     (void)1;
PRINT_MACRO(536);
if(permutedInDomain(PERM_ARGS, 7,1))
{ return; }
else
{ goto Label538; }
Label538:     (void)1;
PRINT_MACRO(538);
permutedRemoveFromDomain(PERM_ARGS, 0,0);
return;
Label540:     (void)1;
PRINT_MACRO(540);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label541; }
else
{ return; }
Label541:     (void)1;
PRINT_MACRO(541);
if(permutedInDomain(PERM_ARGS, 7,0))
{ return; }
else
{ goto Label543; }
Label543:     (void)1;
PRINT_MACRO(543);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
return;
Label545:     (void)1;
PRINT_MACRO(545);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label546; }
else
{ goto Label549; }
Label546:     (void)1;
PRINT_MACRO(546);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label547; }
else
{ goto Label548; }
Label547:     (void)1;
PRINT_MACRO(547);
{
const int new_perm[20] = { 0, 1, 10, 11, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label389;
return;
Label548:     (void)1;
PRINT_MACRO(548);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label494;
return;
Label549:     (void)1;
PRINT_MACRO(549);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label550; }
else
{ goto Label559; }
Label550:     (void)1;
PRINT_MACRO(550);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label551; }
else
{ goto Label552; }
Label551:     (void)1;
PRINT_MACRO(551);
{
const int new_perm[20] = { 0, 1, 12, 13, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label393;
return;
Label552:     (void)1;
PRINT_MACRO(552);
if(permutedInDomain(PERM_ARGS, 9,0))
{ goto Label553; }
else
{ goto Label554; }
Label553:     (void)1;
PRINT_MACRO(553);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label507;
return;
Label554:     (void)1;
PRINT_MACRO(554);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label555; }
else
{ goto Label558; }
Label555:     (void)1;
PRINT_MACRO(555);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label556; }
else
{ goto Label557; }
Label556:     (void)1;
PRINT_MACRO(556);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label528;
return;
Label557:     (void)1;
PRINT_MACRO(557);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label541;
return;
Label558:     (void)1;
PRINT_MACRO(558);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15, 8, 9, 10, 11, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label489;
return;
Label559:     (void)1;
PRINT_MACRO(559);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label560; }
else
{ goto Label573; }
Label560:     (void)1;
PRINT_MACRO(560);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label561; }
else
{ goto Label562; }
Label561:     (void)1;
PRINT_MACRO(561);
{
const int new_perm[20] = { 0, 1, 14, 15, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label401;
return;
Label562:     (void)1;
PRINT_MACRO(562);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label563; }
else
{ goto Label570; }
Label563:     (void)1;
PRINT_MACRO(563);
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label564; }
else
{ goto Label567; }
Label564:     (void)1;
PRINT_MACRO(564);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label566; }
Label566:     (void)1;
PRINT_MACRO(566);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
return;
Label567:     (void)1;
PRINT_MACRO(567);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label569; }
Label569:     (void)1;
PRINT_MACRO(569);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
return;
Label570:     (void)1;
PRINT_MACRO(570);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label572; }
Label572:     (void)1;
PRINT_MACRO(572);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
return;
Label573:     (void)1;
PRINT_MACRO(573);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label574:     (void)1;
PRINT_MACRO(574);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label575; }
else
{ goto Label578; }
Label575:     (void)1;
PRINT_MACRO(575);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label576; }
else
{ goto Label577; }
Label576:     (void)1;
PRINT_MACRO(576);
{
const int new_perm[20] = { 0, 1, 8, 9, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label412;
return;
Label577:     (void)1;
PRINT_MACRO(577);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 8, 9, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label545;
return;
Label578:     (void)1;
PRINT_MACRO(578);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label579; }
else
{ goto Label582; }
Label579:     (void)1;
PRINT_MACRO(579);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label580; }
else
{ goto Label581; }
Label580:     (void)1;
PRINT_MACRO(580);
{
const int new_perm[20] = { 0, 1, 10, 11, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label416;
return;
Label581:     (void)1;
PRINT_MACRO(581);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 10, 11, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label549;
return;
Label582:     (void)1;
PRINT_MACRO(582);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label583; }
else
{ goto Label586; }
Label583:     (void)1;
PRINT_MACRO(583);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label584; }
else
{ goto Label585; }
Label584:     (void)1;
PRINT_MACRO(584);
{
const int new_perm[20] = { 0, 1, 12, 13, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label420;
return;
Label585:     (void)1;
PRINT_MACRO(585);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 12, 13, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label559;
return;
Label586:     (void)1;
PRINT_MACRO(586);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label587:     (void)1;
PRINT_MACRO(587);
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label588; }
else
{ goto Label591; }
Label588:     (void)1;
PRINT_MACRO(588);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label589; }
else
{ goto Label590; }
Label589:     (void)1;
PRINT_MACRO(589);
{
const int new_perm[20] = { 0, 1, 6, 7, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label425;
return;
Label590:     (void)1;
PRINT_MACRO(590);
{
const int new_perm[20] = { 0, 1, 2, 3, 6, 7, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label574;
return;
Label591:     (void)1;
PRINT_MACRO(591);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label592; }
else
{ goto Label595; }
Label592:     (void)1;
PRINT_MACRO(592);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label593; }
else
{ goto Label594; }
Label593:     (void)1;
PRINT_MACRO(593);
{
const int new_perm[20] = { 0, 1, 8, 9, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label429;
return;
Label594:     (void)1;
PRINT_MACRO(594);
{
const int new_perm[20] = { 0, 1, 2, 3, 8, 9, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label578;
return;
Label595:     (void)1;
PRINT_MACRO(595);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label596; }
else
{ goto Label599; }
Label596:     (void)1;
PRINT_MACRO(596);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label597; }
else
{ goto Label598; }
Label597:     (void)1;
PRINT_MACRO(597);
{
const int new_perm[20] = { 0, 1, 10, 11, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label433;
return;
Label598:     (void)1;
PRINT_MACRO(598);
{
const int new_perm[20] = { 0, 1, 2, 3, 10, 11, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label582;
return;
Label599:     (void)1;
PRINT_MACRO(599);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label600:     (void)1;
PRINT_MACRO(600);
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label601; }
else
{ goto Label604; }
Label601:     (void)1;
PRINT_MACRO(601);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label602; }
else
{ goto Label603; }
Label602:     (void)1;
PRINT_MACRO(602);
{
const int new_perm[20] = { 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label438;
return;
Label603:     (void)1;
PRINT_MACRO(603);
{
const int new_perm[20] = { 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label587;
return;
Label604:     (void)1;
PRINT_MACRO(604);
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label605; }
else
{ goto Label608; }
Label605:     (void)1;
PRINT_MACRO(605);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label606; }
else
{ goto Label607; }
Label606:     (void)1;
PRINT_MACRO(606);
{
const int new_perm[20] = { 0, 1, 6, 7, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label442;
return;
Label607:     (void)1;
PRINT_MACRO(607);
{
const int new_perm[20] = { 0, 1, 6, 7, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label591;
return;
Label608:     (void)1;
PRINT_MACRO(608);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label609; }
else
{ goto Label612; }
Label609:     (void)1;
PRINT_MACRO(609);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label610; }
else
{ goto Label611; }
Label610:     (void)1;
PRINT_MACRO(610);
{
const int new_perm[20] = { 0, 1, 8, 9, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label446;
return;
Label611:     (void)1;
PRINT_MACRO(611);
{
const int new_perm[20] = { 0, 1, 8, 9, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label595;
return;
Label612:     (void)1;
PRINT_MACRO(612);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label613:     (void)1;
PRINT_MACRO(613);
if(permutedInDomain(PERM_ARGS, 1,0))
{ goto Label614; }
else
{ goto Label769; }
Label614:     (void)1;
PRINT_MACRO(614);
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label615; }
else
{ goto Label616; }
Label615:     (void)1;
PRINT_MACRO(615);
{
const int new_perm[20] = { 2, 3, 0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label451;
return;
Label616:     (void)1;
PRINT_MACRO(616);
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label617; }
else
{ goto Label756; }
Label617:     (void)1;
PRINT_MACRO(617);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label618; }
else
{ goto Label619; }
Label618:     (void)1;
PRINT_MACRO(618);
{
const int new_perm[20] = { 4, 5, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label454;
return;
Label619:     (void)1;
PRINT_MACRO(619);
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label620; }
else
{ goto Label729; }
Label620:     (void)1;
PRINT_MACRO(620);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label621; }
else
{ goto Label622; }
Label621:     (void)1;
PRINT_MACRO(621);
{
const int new_perm[20] = { 6, 7, 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label457;
return;
Label622:     (void)1;
PRINT_MACRO(622);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label623; }
else
{ goto Label664; }
Label623:     (void)1;
PRINT_MACRO(623);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label624; }
else
{ goto Label625; }
Label624:     (void)1;
PRINT_MACRO(624);
{
const int new_perm[20] = { 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label460;
return;
Label625:     (void)1;
PRINT_MACRO(625);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label626; }
else
{ goto Label643; }
Label626:     (void)1;
PRINT_MACRO(626);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label627; }
else
{ goto Label628; }
Label627:     (void)1;
PRINT_MACRO(627);
{
const int new_perm[20] = { 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label463;
return;
Label628:     (void)1;
PRINT_MACRO(628);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label629; }
else
{ goto Label632; }
Label629:     (void)1;
PRINT_MACRO(629);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label630; }
else
{ goto Label631; }
Label630:     (void)1;
PRINT_MACRO(630);
{
const int new_perm[20] = { 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label466;
return;
Label631:     (void)1;
PRINT_MACRO(631);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label632:     (void)1;
PRINT_MACRO(632);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label633; }
else
{ goto Label636; }
Label633:     (void)1;
PRINT_MACRO(633);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label634; }
else
{ goto Label635; }
Label634:     (void)1;
PRINT_MACRO(634);
{
const int new_perm[20] = { 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label470;
return;
Label635:     (void)1;
PRINT_MACRO(635);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label636:     (void)1;
PRINT_MACRO(636);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label637; }
else
{ goto Label642; }
Label637:     (void)1;
PRINT_MACRO(637);
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label638; }
else
{ goto Label641; }
Label638:     (void)1;
PRINT_MACRO(638);
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label640; }
Label640:     (void)1;
PRINT_MACRO(640);
permutedRemoveFromDomain(PERM_ARGS, 8,0);
return;
Label641:     (void)1;
PRINT_MACRO(641);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label642:     (void)1;
PRINT_MACRO(642);
permutedRemoveFromDomain(PERM_ARGS, 9,0);
return;
Label643:     (void)1;
PRINT_MACRO(643);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label644; }
else
{ goto Label647; }
Label644:     (void)1;
PRINT_MACRO(644);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label645; }
else
{ goto Label646; }
Label645:     (void)1;
PRINT_MACRO(645);
{
const int new_perm[20] = { 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label479;
return;
Label646:     (void)1;
PRINT_MACRO(646);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label632;
return;
Label647:     (void)1;
PRINT_MACRO(647);
if(permutedInDomain(PERM_ARGS, 9,0))
{ goto Label648; }
else
{ goto Label657; }
Label648:     (void)1;
PRINT_MACRO(648);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label649; }
else
{ goto Label656; }
Label649:     (void)1;
PRINT_MACRO(649);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label650; }
else
{ goto Label651; }
Label650:     (void)1;
PRINT_MACRO(650);
{
const int new_perm[20] = { 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label484;
return;
Label651:     (void)1;
PRINT_MACRO(651);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label652; }
else
{ goto Label655; }
Label652:     (void)1;
PRINT_MACRO(652);
if(permutedInDomain(PERM_ARGS, 8,1))
{ return; }
else
{ goto Label654; }
Label654:     (void)1;
PRINT_MACRO(654);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label655:     (void)1;
PRINT_MACRO(655);
permutedRemoveFromDomain(PERM_ARGS, 9,0);
return;
Label656:     (void)1;
PRINT_MACRO(656);
permutedRemoveFromDomain(PERM_ARGS, 9,0);
return;
Label657:     (void)1;
PRINT_MACRO(657);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label658; }
else
{ return; }
Label658:     (void)1;
PRINT_MACRO(658);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label659; }
else
{ return; }
Label659:     (void)1;
PRINT_MACRO(659);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label660; }
else
{ goto Label661; }
Label660:     (void)1;
PRINT_MACRO(660);
{
const int new_perm[20] = { 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label490;
return;
Label661:     (void)1;
PRINT_MACRO(661);
permutedRemoveFromDomain(PERM_ARGS, 8,0);
return;
Label664:     (void)1;
PRINT_MACRO(664);
if(permutedInDomain(PERM_ARGS, 9,0))
{ goto Label665; }
else
{ goto Label686; }
Label665:     (void)1;
PRINT_MACRO(665);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label666; }
else
{ goto Label677; }
Label666:     (void)1;
PRINT_MACRO(666);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label667; }
else
{ goto Label668; }
Label667:     (void)1;
PRINT_MACRO(667);
{
const int new_perm[20] = { 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label495;
return;
Label668:     (void)1;
PRINT_MACRO(668);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label669; }
else
{ goto Label676; }
Label669:     (void)1;
PRINT_MACRO(669);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label670; }
else
{ goto Label671; }
Label670:     (void)1;
PRINT_MACRO(670);
{
const int new_perm[20] = { 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label498;
return;
Label671:     (void)1;
PRINT_MACRO(671);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label672; }
else
{ goto Label675; }
Label672:     (void)1;
PRINT_MACRO(672);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label673; }
else
{ goto Label674; }
Label673:     (void)1;
PRINT_MACRO(673);
{
const int new_perm[20] = { 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label501;
return;
Label674:     (void)1;
PRINT_MACRO(674);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label675:     (void)1;
PRINT_MACRO(675);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9, 14, 15, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label651;
return;
Label676:     (void)1;
PRINT_MACRO(676);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label648;
return;
Label677:     (void)1;
PRINT_MACRO(677);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label678; }
else
{ goto Label681; }
Label678:     (void)1;
PRINT_MACRO(678);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label679; }
else
{ goto Label680; }
Label679:     (void)1;
PRINT_MACRO(679);
{
const int new_perm[20] = { 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label507;
return;
Label680:     (void)1;
PRINT_MACRO(680);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label648;
return;
Label681:     (void)1;
PRINT_MACRO(681);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label682; }
else
{ goto Label685; }
Label682:     (void)1;
PRINT_MACRO(682);
if(permutedInDomain(PERM_ARGS, 7,1))
{ return; }
else
{ goto Label684; }
Label684:     (void)1;
PRINT_MACRO(684);
permutedRemoveFromDomain(PERM_ARGS, 9,0);
return;
Label685:     (void)1;
PRINT_MACRO(685);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label686:     (void)1;
PRINT_MACRO(686);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label687; }
else
{ goto Label712; }
Label687:     (void)1;
PRINT_MACRO(687);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label688; }
else
{ goto Label703; }
Label688:     (void)1;
PRINT_MACRO(688);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label689; }
else
{ goto Label698; }
Label689:     (void)1;
PRINT_MACRO(689);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label690; }
else
{ goto Label695; }
Label690:     (void)1;
PRINT_MACRO(690);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label691; }
else
{ goto Label692; }
Label691:     (void)1;
PRINT_MACRO(691);
{
const int new_perm[20] = { 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label519;
return;
Label692:     (void)1;
PRINT_MACRO(692);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label693; }
else
{ goto Label694; }
Label693:     (void)1;
PRINT_MACRO(693);
{
const int new_perm[20] = { 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label521;
return;
Label694:     (void)1;
PRINT_MACRO(694);
permutedRemoveFromDomain(PERM_ARGS, 7,0);
permutedRemoveFromDomain(PERM_ARGS, 8,0);
return;
Label695:     (void)1;
PRINT_MACRO(695);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label696; }
else
{ goto Label697; }
Label696:     (void)1;
PRINT_MACRO(696);
{
const int new_perm[20] = { 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label524;
return;
Label697:     (void)1;
PRINT_MACRO(697);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9, 14, 15, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label659;
return;
Label698:     (void)1;
PRINT_MACRO(698);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label699; }
else
{ goto Label700; }
Label699:     (void)1;
PRINT_MACRO(699);
{
const int new_perm[20] = { 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label527;
return;
Label700:     (void)1;
PRINT_MACRO(700);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label701; }
else
{ return; }
Label701:     (void)1;
PRINT_MACRO(701);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label659;
return;
Label703:     (void)1;
PRINT_MACRO(703);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label704; }
else
{ goto Label711; }
Label704:     (void)1;
PRINT_MACRO(704);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label705; }
else
{ goto Label708; }
Label705:     (void)1;
PRINT_MACRO(705);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label706; }
else
{ goto Label707; }
Label706:     (void)1;
PRINT_MACRO(706);
{
const int new_perm[20] = { 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label528;
return;
Label707:     (void)1;
PRINT_MACRO(707);
permutedRemoveFromDomain(PERM_ARGS, 7,1);
return;
Label708:     (void)1;
PRINT_MACRO(708);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label709; }
else
{ return; }
Label709:     (void)1;
PRINT_MACRO(709);
{
const int new_perm[20] = { 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 14, 15, 8, 9, 10, 11, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label490;
return;
Label711:     (void)1;
PRINT_MACRO(711);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label700;
return;
Label712:     (void)1;
PRINT_MACRO(712);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label713; }
else
{ goto Label722; }
Label713:     (void)1;
PRINT_MACRO(713);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label714; }
else
{ goto Label715; }
Label714:     (void)1;
PRINT_MACRO(714);
{
const int new_perm[20] = { 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label532;
return;
Label715:     (void)1;
PRINT_MACRO(715);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label716; }
else
{ return; }
Label716:     (void)1;
PRINT_MACRO(716);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label717; }
else
{ return; }
Label717:     (void)1;
PRINT_MACRO(717);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label718; }
else
{ goto Label719; }
Label718:     (void)1;
PRINT_MACRO(718);
{
const int new_perm[20] = { 12, 13, 0, 1, 2, 3, 10, 11, 6, 7, 8, 9, 4, 5, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label536;
return;
Label719:     (void)1;
PRINT_MACRO(719);
permutedRemoveFromDomain(PERM_ARGS, 7,0);
return;
Label722:     (void)1;
PRINT_MACRO(722);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label723; }
else
{ return; }
Label723:     (void)1;
PRINT_MACRO(723);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label724; }
else
{ return; }
Label724:     (void)1;
PRINT_MACRO(724);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label725; }
else
{ goto Label726; }
Label725:     (void)1;
PRINT_MACRO(725);
{
const int new_perm[20] = { 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label541;
return;
Label726:     (void)1;
PRINT_MACRO(726);
permutedRemoveFromDomain(PERM_ARGS, 7,1);
return;
Label729:     (void)1;
PRINT_MACRO(729);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label730; }
else
{ goto Label733; }
Label730:     (void)1;
PRINT_MACRO(730);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label731; }
else
{ goto Label732; }
Label731:     (void)1;
PRINT_MACRO(731);
{
const int new_perm[20] = { 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label545;
return;
Label732:     (void)1;
PRINT_MACRO(732);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 8, 9, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label664;
return;
Label733:     (void)1;
PRINT_MACRO(733);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label734; }
else
{ goto Label747; }
Label734:     (void)1;
PRINT_MACRO(734);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label735; }
else
{ goto Label736; }
Label735:     (void)1;
PRINT_MACRO(735);
{
const int new_perm[20] = { 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label549;
return;
Label736:     (void)1;
PRINT_MACRO(736);
if(permutedInDomain(PERM_ARGS, 9,0))
{ goto Label737; }
else
{ goto Label738; }
Label737:     (void)1;
PRINT_MACRO(737);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 10, 11, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label677;
return;
Label738:     (void)1;
PRINT_MACRO(738);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label739; }
else
{ goto Label746; }
Label739:     (void)1;
PRINT_MACRO(739);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label740; }
else
{ goto Label743; }
Label740:     (void)1;
PRINT_MACRO(740);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label741; }
else
{ goto Label742; }
Label741:     (void)1;
PRINT_MACRO(741);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 10, 11, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label705;
return;
Label742:     (void)1;
PRINT_MACRO(742);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 10, 11, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label724;
return;
Label743:     (void)1;
PRINT_MACRO(743);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label744; }
else
{ return; }
Label744:     (void)1;
PRINT_MACRO(744);
{
const int new_perm[20] = { 12, 13, 0, 1, 2, 3, 4, 5, 10, 11, 14, 15, 6, 7, 8, 9, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label489;
return;
Label746:     (void)1;
PRINT_MACRO(746);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 10, 11, 12, 13, 6, 7, 8, 9, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label657;
return;
Label747:     (void)1;
PRINT_MACRO(747);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label748; }
else
{ goto Label755; }
Label748:     (void)1;
PRINT_MACRO(748);
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label749; }
else
{ goto Label750; }
Label749:     (void)1;
PRINT_MACRO(749);
{
const int new_perm[20] = { 12, 13, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label559;
return;
Label750:     (void)1;
PRINT_MACRO(750);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label751; }
else
{ goto Label754; }
Label751:     (void)1;
PRINT_MACRO(751);
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label752; }
else
{ goto Label753; }
Label752:     (void)1;
PRINT_MACRO(752);
{
const int new_perm[20] = { 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label562;
return;
Label753:     (void)1;
PRINT_MACRO(753);
permutedRemoveFromDomain(PERM_ARGS, 9,0);
return;
Label754:     (void)1;
PRINT_MACRO(754);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label755:     (void)1;
PRINT_MACRO(755);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label756:     (void)1;
PRINT_MACRO(756);
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label757; }
else
{ goto Label760; }
Label757:     (void)1;
PRINT_MACRO(757);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label758; }
else
{ goto Label759; }
Label758:     (void)1;
PRINT_MACRO(758);
{
const int new_perm[20] = { 6, 7, 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label574;
return;
Label759:     (void)1;
PRINT_MACRO(759);
{
const int new_perm[20] = { 0, 1, 2, 3, 6, 7, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label729;
return;
Label760:     (void)1;
PRINT_MACRO(760);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label761; }
else
{ goto Label764; }
Label761:     (void)1;
PRINT_MACRO(761);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label762; }
else
{ goto Label763; }
Label762:     (void)1;
PRINT_MACRO(762);
{
const int new_perm[20] = { 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label578;
return;
Label763:     (void)1;
PRINT_MACRO(763);
{
const int new_perm[20] = { 0, 1, 2, 3, 8, 9, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label733;
return;
Label764:     (void)1;
PRINT_MACRO(764);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label765; }
else
{ goto Label768; }
Label765:     (void)1;
PRINT_MACRO(765);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label766; }
else
{ goto Label767; }
Label766:     (void)1;
PRINT_MACRO(766);
{
const int new_perm[20] = { 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label582;
return;
Label767:     (void)1;
PRINT_MACRO(767);
{
const int new_perm[20] = { 0, 1, 2, 3, 10, 11, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label747;
return;
Label768:     (void)1;
PRINT_MACRO(768);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label769:     (void)1;
PRINT_MACRO(769);
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label770; }
else
{ goto Label773; }
Label770:     (void)1;
PRINT_MACRO(770);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label771; }
else
{ goto Label772; }
Label771:     (void)1;
PRINT_MACRO(771);
{
const int new_perm[20] = { 4, 5, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label587;
return;
Label772:     (void)1;
PRINT_MACRO(772);
{
const int new_perm[20] = { 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label756;
return;
Label773:     (void)1;
PRINT_MACRO(773);
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label774; }
else
{ goto Label777; }
Label774:     (void)1;
PRINT_MACRO(774);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label775; }
else
{ goto Label776; }
Label775:     (void)1;
PRINT_MACRO(775);
{
const int new_perm[20] = { 6, 7, 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label591;
return;
Label776:     (void)1;
PRINT_MACRO(776);
{
const int new_perm[20] = { 0, 1, 6, 7, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label760;
return;
Label777:     (void)1;
PRINT_MACRO(777);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label778; }
else
{ goto Label781; }
Label778:     (void)1;
PRINT_MACRO(778);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label779; }
else
{ goto Label780; }
Label779:     (void)1;
PRINT_MACRO(779);
{
const int new_perm[20] = { 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label595;
return;
Label780:     (void)1;
PRINT_MACRO(780);
{
const int new_perm[20] = { 0, 1, 8, 9, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label764;
return;
Label781:     (void)1;
PRINT_MACRO(781);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label782:     (void)1;
PRINT_MACRO(782);
if(permutedInDomain(PERM_ARGS, 1,0))
{ goto Label783; }
else
{ goto Label786; }
Label783:     (void)1;
PRINT_MACRO(783);
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label784; }
else
{ goto Label785; }
Label784:     (void)1;
PRINT_MACRO(784);
{
const int new_perm[20] = { 2, 3, 0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label600;
return;
Label785:     (void)1;
PRINT_MACRO(785);
{
const int new_perm[20] = { 2, 3, 0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label769;
return;
Label786:     (void)1;
PRINT_MACRO(786);
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label787; }
else
{ goto Label790; }
Label787:     (void)1;
PRINT_MACRO(787);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label788; }
else
{ goto Label789; }
Label788:     (void)1;
PRINT_MACRO(788);
{
const int new_perm[20] = { 4, 5, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label604;
return;
Label789:     (void)1;
PRINT_MACRO(789);
{
const int new_perm[20] = { 4, 5, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label773;
return;
Label790:     (void)1;
PRINT_MACRO(790);
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label791; }
else
{ goto Label794; }
Label791:     (void)1;
PRINT_MACRO(791);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label792; }
else
{ goto Label793; }
Label792:     (void)1;
PRINT_MACRO(792);
{
const int new_perm[20] = { 6, 7, 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label608;
return;
Label793:     (void)1;
PRINT_MACRO(793);
{
const int new_perm[20] = { 6, 7, 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label777;
return;
Label794:     (void)1;
PRINT_MACRO(794);
permutedRemoveFromDomain(PERM_ARGS, 9,1);
return;
Label795:     (void)1;
PRINT_MACRO(795);
if(permutedInDomain(PERM_ARGS, 0,0))
{ goto Label796; }
else
{ goto Label1423; }
Label796:     (void)1;
PRINT_MACRO(796);
if(permutedInDomain(PERM_ARGS, 1,0))
{ goto Label797; }
else
{ goto Label1416; }
Label797:     (void)1;
PRINT_MACRO(797);
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label798; }
else
{ goto Label1409; }
Label798:     (void)1;
PRINT_MACRO(798);
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label799; }
else
{ goto Label1402; }
Label799:     (void)1;
PRINT_MACRO(799);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label800; }
else
{ goto Label1395; }
Label800:     (void)1;
PRINT_MACRO(800);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label801; }
else
{ goto Label1302; }
Label801:     (void)1;
PRINT_MACRO(801);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label802; }
else
{ goto Label1179; }
Label802:     (void)1;
PRINT_MACRO(802);
if(permutedInDomain(PERM_ARGS, 7,0))
{ return; }
else
{ goto Label1014; }
Label1014:     (void)1;
PRINT_MACRO(1014);
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label1015; }
else
{ return; }
Label1015:     (void)1;
PRINT_MACRO(1015);
if(permutedInDomain(PERM_ARGS, 8,0))
{ return; }
else
{ goto Label1073; }
Label1073:     (void)1;
PRINT_MACRO(1073);
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label1074; }
else
{ goto Label1115; }
Label1074:     (void)1;
PRINT_MACRO(1074);
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1075; }
else
{ goto Label1106; }
Label1075:     (void)1;
PRINT_MACRO(1075);
if(permutedInDomain(PERM_ARGS, 2,1))
{ return; }
else
{ goto Label1097; }
Label1097:     (void)1;
PRINT_MACRO(1097);
if(permutedInDomain(PERM_ARGS, 3,1))
{ return; }
else
{ goto Label1099; }
Label1099:     (void)1;
PRINT_MACRO(1099);
if(permutedInDomain(PERM_ARGS, 4,1))
{ return; }
else
{ goto Label1101; }
Label1101:     (void)1;
PRINT_MACRO(1101);
if(permutedInDomain(PERM_ARGS, 5,1))
{ return; }
else
{ goto Label1103; }
Label1103:     (void)1;
PRINT_MACRO(1103);
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label1105; }
Label1105:     (void)1;
PRINT_MACRO(1105);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
return;
Label1106:     (void)1;
PRINT_MACRO(1106);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1107; }
else
{ goto Label1108; }
Label1107:     (void)1;
PRINT_MACRO(1107);
{
const int new_perm[20] = { 4, 5, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1097;
return;
Label1108:     (void)1;
PRINT_MACRO(1108);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1109; }
else
{ goto Label1110; }
Label1109:     (void)1;
PRINT_MACRO(1109);
{
const int new_perm[20] = { 6, 7, 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1099;
return;
Label1110:     (void)1;
PRINT_MACRO(1110);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label1111; }
else
{ goto Label1112; }
Label1111:     (void)1;
PRINT_MACRO(1111);
{
const int new_perm[20] = { 8, 9, 0, 1, 6, 7, 2, 3, 4, 5, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1101;
return;
Label1112:     (void)1;
PRINT_MACRO(1112);
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label1113; }
else
{ goto Label1114; }
Label1113:     (void)1;
PRINT_MACRO(1113);
{
const int new_perm[20] = { 0, 1, 10, 11, 2, 3, 4, 5, 8, 9, 6, 7, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1103;
return;
Label1114:     (void)1;
PRINT_MACRO(1114);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 6,1);
return;
Label1115:     (void)1;
PRINT_MACRO(1115);
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1116; }
else
{ goto Label1117; }
Label1116:     (void)1;
PRINT_MACRO(1116);
{
const int new_perm[20] = { 2, 3, 0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1106;
return;
Label1117:     (void)1;
PRINT_MACRO(1117);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1118; }
else
{ goto Label1119; }
Label1118:     (void)1;
PRINT_MACRO(1118);
{
const int new_perm[20] = { 4, 5, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1108;
return;
Label1119:     (void)1;
PRINT_MACRO(1119);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1120; }
else
{ goto Label1121; }
Label1120:     (void)1;
PRINT_MACRO(1120);
{
const int new_perm[20] = { 6, 7, 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1110;
return;
Label1121:     (void)1;
PRINT_MACRO(1121);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label1122; }
else
{ goto Label1123; }
Label1122:     (void)1;
PRINT_MACRO(1122);
{
const int new_perm[20] = { 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1112;
return;
Label1123:     (void)1;
PRINT_MACRO(1123);
permutedRemoveFromDomain(PERM_ARGS, 5,1);
permutedRemoveFromDomain(PERM_ARGS, 6,1);
return;
Label1179:     (void)1;
PRINT_MACRO(1179);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label1180; }
else
{ goto Label1181; }
Label1180:     (void)1;
PRINT_MACRO(1180);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1014;
return;
Label1181:     (void)1;
PRINT_MACRO(1181);
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label1182; }
else
{ goto Label1261; }
Label1182:     (void)1;
PRINT_MACRO(1182);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label1183; }
else
{ goto Label1224; }
Label1183:     (void)1;
PRINT_MACRO(1183);
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label1184; }
else
{ goto Label1215; }
Label1184:     (void)1;
PRINT_MACRO(1184);
if(permutedInDomain(PERM_ARGS, 1,1))
{ return; }
else
{ goto Label1206; }
Label1206:     (void)1;
PRINT_MACRO(1206);
if(permutedInDomain(PERM_ARGS, 2,1))
{ return; }
else
{ goto Label1208; }
Label1208:     (void)1;
PRINT_MACRO(1208);
if(permutedInDomain(PERM_ARGS, 3,1))
{ return; }
else
{ goto Label1210; }
Label1210:     (void)1;
PRINT_MACRO(1210);
if(permutedInDomain(PERM_ARGS, 4,1))
{ return; }
else
{ goto Label1212; }
Label1212:     (void)1;
PRINT_MACRO(1212);
if(permutedInDomain(PERM_ARGS, 5,1))
{ return; }
else
{ goto Label1214; }
Label1214:     (void)1;
PRINT_MACRO(1214);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 8,1);
return;
Label1215:     (void)1;
PRINT_MACRO(1215);
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1216; }
else
{ goto Label1217; }
Label1216:     (void)1;
PRINT_MACRO(1216);
{
const int new_perm[20] = { 2, 3, 0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1206;
return;
Label1217:     (void)1;
PRINT_MACRO(1217);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1218; }
else
{ goto Label1219; }
Label1218:     (void)1;
PRINT_MACRO(1218);
{
const int new_perm[20] = { 4, 5, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1208;
return;
Label1219:     (void)1;
PRINT_MACRO(1219);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1220; }
else
{ goto Label1221; }
Label1220:     (void)1;
PRINT_MACRO(1220);
{
const int new_perm[20] = { 6, 7, 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1210;
return;
Label1221:     (void)1;
PRINT_MACRO(1221);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label1222; }
else
{ goto Label1223; }
Label1222:     (void)1;
PRINT_MACRO(1222);
{
const int new_perm[20] = { 8, 9, 0, 1, 2, 3, 6, 7, 4, 5, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1212;
return;
Label1223:     (void)1;
PRINT_MACRO(1223);
permutedRemoveFromDomain(PERM_ARGS, 5,1);
permutedRemoveFromDomain(PERM_ARGS, 8,1);
return;
Label1224:     (void)1;
PRINT_MACRO(1224);
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label1225; }
else
{ goto Label1254; }
Label1225:     (void)1;
PRINT_MACRO(1225);
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1226; }
else
{ goto Label1247; }
Label1226:     (void)1;
PRINT_MACRO(1226);
if(permutedInDomain(PERM_ARGS, 2,1))
{ return; }
else
{ goto Label1240; }
Label1240:     (void)1;
PRINT_MACRO(1240);
if(permutedInDomain(PERM_ARGS, 3,1))
{ return; }
else
{ goto Label1242; }
Label1242:     (void)1;
PRINT_MACRO(1242);
if(permutedInDomain(PERM_ARGS, 4,1))
{ return; }
else
{ goto Label1244; }
Label1244:     (void)1;
PRINT_MACRO(1244);
if(permutedInDomain(PERM_ARGS, 5,1))
{ return; }
else
{ goto Label1246; }
Label1246:     (void)1;
PRINT_MACRO(1246);
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 1,0);
return;
Label1247:     (void)1;
PRINT_MACRO(1247);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1248; }
else
{ goto Label1249; }
Label1248:     (void)1;
PRINT_MACRO(1248);
{
const int new_perm[20] = { 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 10, 11, 14, 15, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1240;
return;
Label1249:     (void)1;
PRINT_MACRO(1249);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1250; }
else
{ goto Label1251; }
Label1250:     (void)1;
PRINT_MACRO(1250);
{
const int new_perm[20] = { 6, 7, 0, 1, 4, 5, 2, 3, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1242;
return;
Label1251:     (void)1;
PRINT_MACRO(1251);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label1252; }
else
{ goto Label1253; }
Label1252:     (void)1;
PRINT_MACRO(1252);
{
const int new_perm[20] = { 0, 1, 8, 9, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1244;
return;
Label1253:     (void)1;
PRINT_MACRO(1253);
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 5,0);
return;
Label1254:     (void)1;
PRINT_MACRO(1254);
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1255; }
else
{ goto Label1256; }
Label1255:     (void)1;
PRINT_MACRO(1255);
{
const int new_perm[20] = { 2, 3, 0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1247;
return;
Label1256:     (void)1;
PRINT_MACRO(1256);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1257; }
else
{ goto Label1258; }
Label1257:     (void)1;
PRINT_MACRO(1257);
{
const int new_perm[20] = { 4, 5, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1249;
return;
Label1258:     (void)1;
PRINT_MACRO(1258);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1259; }
else
{ goto Label1260; }
Label1259:     (void)1;
PRINT_MACRO(1259);
{
const int new_perm[20] = { 6, 7, 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1251;
return;
Label1260:     (void)1;
PRINT_MACRO(1260);
permutedRemoveFromDomain(PERM_ARGS, 4,0);
permutedRemoveFromDomain(PERM_ARGS, 5,0);
return;
Label1261:     (void)1;
PRINT_MACRO(1261);
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label1262; }
else
{ goto Label1293; }
Label1262:     (void)1;
PRINT_MACRO(1262);
if(permutedInDomain(PERM_ARGS, 1,1))
{ return; }
else
{ goto Label1284; }
Label1284:     (void)1;
PRINT_MACRO(1284);
if(permutedInDomain(PERM_ARGS, 2,1))
{ return; }
else
{ goto Label1286; }
Label1286:     (void)1;
PRINT_MACRO(1286);
if(permutedInDomain(PERM_ARGS, 3,1))
{ return; }
else
{ goto Label1288; }
Label1288:     (void)1;
PRINT_MACRO(1288);
if(permutedInDomain(PERM_ARGS, 4,1))
{ return; }
else
{ goto Label1290; }
Label1290:     (void)1;
PRINT_MACRO(1290);
if(permutedInDomain(PERM_ARGS, 5,1))
{ return; }
else
{ goto Label1292; }
Label1292:     (void)1;
PRINT_MACRO(1292);
permutedRemoveFromDomain(PERM_ARGS, 0,1);
return;
Label1293:     (void)1;
PRINT_MACRO(1293);
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1294; }
else
{ goto Label1295; }
Label1294:     (void)1;
PRINT_MACRO(1294);
{
const int new_perm[20] = { 2, 3, 0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1284;
return;
Label1295:     (void)1;
PRINT_MACRO(1295);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1296; }
else
{ goto Label1297; }
Label1296:     (void)1;
PRINT_MACRO(1296);
{
const int new_perm[20] = { 4, 5, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1286;
return;
Label1297:     (void)1;
PRINT_MACRO(1297);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1298; }
else
{ goto Label1299; }
Label1298:     (void)1;
PRINT_MACRO(1298);
{
const int new_perm[20] = { 6, 7, 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1288;
return;
Label1299:     (void)1;
PRINT_MACRO(1299);
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label1300; }
else
{ goto Label1301; }
Label1300:     (void)1;
PRINT_MACRO(1300);
{
const int new_perm[20] = { 8, 9, 0, 1, 2, 3, 6, 7, 4, 5, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1290;
return;
Label1301:     (void)1;
PRINT_MACRO(1301);
permutedRemoveFromDomain(PERM_ARGS, 5,1);
return;
Label1302:     (void)1;
PRINT_MACRO(1302);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label1303; }
else
{ goto Label1304; }
Label1303:     (void)1;
PRINT_MACRO(1303);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1179;
return;
Label1304:     (void)1;
PRINT_MACRO(1304);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label1305; }
else
{ goto Label1306; }
Label1305:     (void)1;
PRINT_MACRO(1305);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 14, 15, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1181;
return;
Label1306:     (void)1;
PRINT_MACRO(1306);
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label1307; }
else
{ goto Label1366; }
Label1307:     (void)1;
PRINT_MACRO(1307);
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label1308; }
else
{ goto Label1337; }
Label1308:     (void)1;
PRINT_MACRO(1308);
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label1309; }
else
{ goto Label1330; }
Label1309:     (void)1;
PRINT_MACRO(1309);
if(permutedInDomain(PERM_ARGS, 1,1))
{ return; }
else
{ goto Label1323; }
Label1323:     (void)1;
PRINT_MACRO(1323);
if(permutedInDomain(PERM_ARGS, 2,1))
{ return; }
else
{ goto Label1325; }
Label1325:     (void)1;
PRINT_MACRO(1325);
if(permutedInDomain(PERM_ARGS, 3,1))
{ return; }
else
{ goto Label1327; }
Label1327:     (void)1;
PRINT_MACRO(1327);
if(permutedInDomain(PERM_ARGS, 4,1))
{ return; }
else
{ goto Label1329; }
Label1329:     (void)1;
PRINT_MACRO(1329);
permutedRemoveFromDomain(PERM_ARGS, 0,0);
return;
Label1330:     (void)1;
PRINT_MACRO(1330);
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1331; }
else
{ goto Label1332; }
Label1331:     (void)1;
PRINT_MACRO(1331);
{
const int new_perm[20] = { 2, 3, 0, 1, 4, 5, 6, 7, 8, 9, 14, 15, 12, 13, 10, 11, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1323;
return;
Label1332:     (void)1;
PRINT_MACRO(1332);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1333; }
else
{ goto Label1334; }
Label1333:     (void)1;
PRINT_MACRO(1333);
{
const int new_perm[20] = { 4, 5, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1325;
return;
Label1334:     (void)1;
PRINT_MACRO(1334);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1335; }
else
{ goto Label1336; }
Label1335:     (void)1;
PRINT_MACRO(1335);
{
const int new_perm[20] = { 6, 7, 0, 1, 2, 3, 4, 5, 8, 9, 14, 15, 12, 13, 10, 11, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1327;
return;
Label1336:     (void)1;
PRINT_MACRO(1336);
permutedRemoveFromDomain(PERM_ARGS, 4,0);
return;
Label1337:     (void)1;
PRINT_MACRO(1337);
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label1338; }
else
{ goto Label1359; }
Label1338:     (void)1;
PRINT_MACRO(1338);
if(permutedInDomain(PERM_ARGS, 1,1))
{ return; }
else
{ goto Label1352; }
Label1352:     (void)1;
PRINT_MACRO(1352);
if(permutedInDomain(PERM_ARGS, 2,1))
{ return; }
else
{ goto Label1354; }
Label1354:     (void)1;
PRINT_MACRO(1354);
if(permutedInDomain(PERM_ARGS, 3,1))
{ return; }
else
{ goto Label1356; }
Label1356:     (void)1;
PRINT_MACRO(1356);
if(permutedInDomain(PERM_ARGS, 4,1))
{ return; }
else
{ goto Label1358; }
Label1358:     (void)1;
PRINT_MACRO(1358);
permutedRemoveFromDomain(PERM_ARGS, 0,0);
return;
Label1359:     (void)1;
PRINT_MACRO(1359);
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1360; }
else
{ goto Label1361; }
Label1360:     (void)1;
PRINT_MACRO(1360);
{
const int new_perm[20] = { 2, 3, 0, 1, 4, 5, 6, 7, 8, 9, 14, 15, 12, 13, 10, 11, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1352;
return;
Label1361:     (void)1;
PRINT_MACRO(1361);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1362; }
else
{ goto Label1363; }
Label1362:     (void)1;
PRINT_MACRO(1362);
{
const int new_perm[20] = { 4, 5, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1354;
return;
Label1363:     (void)1;
PRINT_MACRO(1363);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1364; }
else
{ goto Label1365; }
Label1364:     (void)1;
PRINT_MACRO(1364);
{
const int new_perm[20] = { 6, 7, 0, 1, 2, 3, 4, 5, 8, 9, 14, 15, 12, 13, 10, 11, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1356;
return;
Label1365:     (void)1;
PRINT_MACRO(1365);
permutedRemoveFromDomain(PERM_ARGS, 4,0);
return;
Label1366:     (void)1;
PRINT_MACRO(1366);
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label1367; }
else
{ goto Label1388; }
Label1367:     (void)1;
PRINT_MACRO(1367);
if(permutedInDomain(PERM_ARGS, 1,1))
{ return; }
else
{ goto Label1381; }
Label1381:     (void)1;
PRINT_MACRO(1381);
if(permutedInDomain(PERM_ARGS, 2,1))
{ return; }
else
{ goto Label1383; }
Label1383:     (void)1;
PRINT_MACRO(1383);
if(permutedInDomain(PERM_ARGS, 3,1))
{ return; }
else
{ goto Label1385; }
Label1385:     (void)1;
PRINT_MACRO(1385);
if(permutedInDomain(PERM_ARGS, 4,1))
{ return; }
else
{ goto Label1387; }
Label1387:     (void)1;
PRINT_MACRO(1387);
permutedRemoveFromDomain(PERM_ARGS, 0,0);
return;
Label1388:     (void)1;
PRINT_MACRO(1388);
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1389; }
else
{ goto Label1390; }
Label1389:     (void)1;
PRINT_MACRO(1389);
{
const int new_perm[20] = { 2, 3, 0, 1, 4, 5, 6, 7, 8, 9, 14, 15, 12, 13, 10, 11, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1381;
return;
Label1390:     (void)1;
PRINT_MACRO(1390);
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1391; }
else
{ goto Label1392; }
Label1391:     (void)1;
PRINT_MACRO(1391);
{
const int new_perm[20] = { 4, 5, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1383;
return;
Label1392:     (void)1;
PRINT_MACRO(1392);
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1393; }
else
{ goto Label1394; }
Label1393:     (void)1;
PRINT_MACRO(1393);
{
const int new_perm[20] = { 6, 7, 0, 1, 2, 3, 4, 5, 8, 9, 14, 15, 12, 13, 10, 11, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1385;
return;
Label1394:     (void)1;
PRINT_MACRO(1394);
permutedRemoveFromDomain(PERM_ARGS, 4,0);
return;
Label1395:     (void)1;
PRINT_MACRO(1395);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label1396; }
else
{ goto Label1397; }
Label1396:     (void)1;
PRINT_MACRO(1396);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1302;
return;
Label1397:     (void)1;
PRINT_MACRO(1397);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label1398; }
else
{ goto Label1399; }
Label1398:     (void)1;
PRINT_MACRO(1398);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1304;
return;
Label1399:     (void)1;
PRINT_MACRO(1399);
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label1400; }
else
{ return; }
Label1400:     (void)1;
PRINT_MACRO(1400);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 14, 15, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1306;
return;
Label1402:     (void)1;
PRINT_MACRO(1402);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label1403; }
else
{ goto Label1404; }
Label1403:     (void)1;
PRINT_MACRO(1403);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 8, 9, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1395;
return;
Label1404:     (void)1;
PRINT_MACRO(1404);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label1405; }
else
{ goto Label1406; }
Label1405:     (void)1;
PRINT_MACRO(1405);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 10, 11, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1397;
return;
Label1406:     (void)1;
PRINT_MACRO(1406);
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label1407; }
else
{ return; }
Label1407:     (void)1;
PRINT_MACRO(1407);
{
const int new_perm[20] = { 0, 1, 2, 3, 4, 5, 12, 13, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1399;
return;
Label1409:     (void)1;
PRINT_MACRO(1409);
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label1410; }
else
{ goto Label1411; }
Label1410:     (void)1;
PRINT_MACRO(1410);
{
const int new_perm[20] = { 0, 1, 2, 3, 6, 7, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1402;
return;
Label1411:     (void)1;
PRINT_MACRO(1411);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label1412; }
else
{ goto Label1413; }
Label1412:     (void)1;
PRINT_MACRO(1412);
{
const int new_perm[20] = { 0, 1, 2, 3, 8, 9, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1404;
return;
Label1413:     (void)1;
PRINT_MACRO(1413);
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label1414; }
else
{ return; }
Label1414:     (void)1;
PRINT_MACRO(1414);
{
const int new_perm[20] = { 0, 1, 2, 3, 10, 11, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1406;
return;
Label1416:     (void)1;
PRINT_MACRO(1416);
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label1417; }
else
{ goto Label1418; }
Label1417:     (void)1;
PRINT_MACRO(1417);
{
const int new_perm[20] = { 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1409;
return;
Label1418:     (void)1;
PRINT_MACRO(1418);
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label1419; }
else
{ goto Label1420; }
Label1419:     (void)1;
PRINT_MACRO(1419);
{
const int new_perm[20] = { 0, 1, 6, 7, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1411;
return;
Label1420:     (void)1;
PRINT_MACRO(1420);
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label1421; }
else
{ return; }
Label1421:     (void)1;
PRINT_MACRO(1421);
{
const int new_perm[20] = { 0, 1, 8, 9, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1413;
return;
Label1423:     (void)1;
PRINT_MACRO(1423);
if(permutedInDomain(PERM_ARGS, 1,0))
{ goto Label1424; }
else
{ goto Label1425; }
Label1424:     (void)1;
PRINT_MACRO(1424);
{
const int new_perm[20] = { 2, 3, 0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1416;
return;
Label1425:     (void)1;
PRINT_MACRO(1425);
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label1426; }
else
{ goto Label1427; }
Label1426:     (void)1;
PRINT_MACRO(1426);
{
const int new_perm[20] = { 4, 5, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1418;
return;
Label1427:     (void)1;
PRINT_MACRO(1427);
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label1428; }
else
{ return; }
Label1428:     (void)1;
PRINT_MACRO(1428);
{
const int new_perm[20] = { 6, 7, 0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1420;
return;
}
#endif

//  Depth: 21
//  Number of nodes: 651
//  Number of nodes explored by algorithm: 1429
