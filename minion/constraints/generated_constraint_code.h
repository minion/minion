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
if(permutedInDomain(PERM_ARGS, 9,1))
{ goto Label2; }
else
{ goto Label795; }
Label2:     (void)1;
if(permutedInDomain(PERM_ARGS, 0,0))
{ goto Label3; }
else
{ goto Label782; }
Label3:     (void)1;
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label4; }
else
{ goto Label613; }
Label4:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,0))
{ goto Label5; }
else
{ goto Label600; }
Label5:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label6; }
else
{ goto Label451; }
Label6:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label7; }
else
{ goto Label438; }
Label7:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label8; }
else
{ goto Label325; }
Label8:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label9; }
else
{ goto Label312; }
Label9:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label10; }
else
{ goto Label231; }
Label10:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label11; }
else
{ goto Label218; }
Label11:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label12; }
else
{ goto Label153; }
Label12:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ return; }
else
{ goto Label134; }
Label134:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ return; }
else
{ goto Label138; }
Label138:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ return; }
else
{ goto Label142; }
Label142:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label143; }
else
{ goto Label150; }
Label143:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label144; }
else
{ goto Label147; }
Label144:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label146; }
Label146:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
permutedRemoveFromDomain(PERM_ARGS, 3,1);
permutedRemoveFromDomain(PERM_ARGS, 4,1);
Label147:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label149; }
Label149:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
permutedRemoveFromDomain(PERM_ARGS, 3,1);
permutedRemoveFromDomain(PERM_ARGS, 4,1);
Label150:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label152; }
Label152:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
permutedRemoveFromDomain(PERM_ARGS, 3,1);
permutedRemoveFromDomain(PERM_ARGS, 4,1);
Label153:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ return; }
else
{ goto Label199; }
Label199:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ return; }
else
{ goto Label203; }
Label203:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ return; }
else
{ goto Label207; }
Label207:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label208; }
else
{ goto Label215; }
Label208:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label209; }
else
{ goto Label212; }
Label209:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label211; }
Label211:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
permutedRemoveFromDomain(PERM_ARGS, 3,1);
Label212:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label214; }
Label214:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
permutedRemoveFromDomain(PERM_ARGS, 3,1);
Label215:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label217; }
Label217:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
permutedRemoveFromDomain(PERM_ARGS, 3,1);
Label218:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label219; }
else
{ goto Label222; }
Label219:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label220; }
else
{ goto Label221; }
Label220:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label134;
Label221:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label199;
Label222:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label223; }
else
{ goto Label226; }
Label223:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label224; }
else
{ goto Label225; }
Label224:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label138;
Label225:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label203;
Label226:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label227; }
else
{ goto Label230; }
Label227:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label228; }
else
{ goto Label229; }
Label228:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 15, 16, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label142;
Label229:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 15, 16, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label207;
Label230:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label231:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label232; }
else
{ goto Label299; }
Label232:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label233; }
else
{ goto Label234; }
Label233:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 9, 10, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label153;
Label234:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label235; }
else
{ goto Label280; }
Label235:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ return; }
else
{ goto Label237; }
Label237:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label238; }
else
{ return; }
Label238:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label240; }
Label240:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label241; }
else
{ return; }
Label241:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ return; }
else
{ goto Label243; }
Label243:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label244; }
else
{ return; }
Label244:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ return; }
else
{ goto Label248; }
Label248:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label250; }
Label250:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 1,0);
permutedRemoveFromDomain(PERM_ARGS, 2,0);
Label280:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ return; }
else
{ goto Label284; }
Label284:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ return; }
else
{ goto Label288; }
Label288:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label289; }
else
{ goto Label296; }
Label289:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label290; }
else
{ goto Label293; }
Label290:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label292; }
Label292:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
Label293:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label295; }
Label295:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
Label296:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label298; }
Label298:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
permutedRemoveFromDomain(PERM_ARGS, 2,1);
Label299:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label300; }
else
{ goto Label303; }
Label300:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label301; }
else
{ goto Label302; }
Label301:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 11, 12, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label199;
Label302:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label280;
Label303:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label304; }
else
{ goto Label307; }
Label304:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label305; }
else
{ goto Label306; }
Label305:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 13, 14, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label203;
Label306:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label284;
Label307:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label308; }
else
{ goto Label311; }
Label308:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label309; }
else
{ goto Label310; }
Label309:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 15, 16, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label207;
Label310:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 15, 16, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label288;
Label311:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label312:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label313; }
else
{ goto Label316; }
Label313:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label314; }
else
{ goto Label315; }
Label314:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 9, 10, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label218;
Label315:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 9, 10, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label299;
Label316:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label317; }
else
{ goto Label320; }
Label317:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label318; }
else
{ goto Label319; }
Label318:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 11, 12, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label222;
Label319:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 11, 12, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label303;
Label320:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label321; }
else
{ goto Label324; }
Label321:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label322; }
else
{ goto Label323; }
Label322:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 13, 14, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label226;
Label323:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 13, 14, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label307;
Label324:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label325:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label326; }
else
{ goto Label425; }
Label326:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label327; }
else
{ goto Label328; }
Label327:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 7, 8, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label231;
Label328:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label329; }
else
{ goto Label412; }
Label329:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label330; }
else
{ goto Label331; }
Label330:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 9, 10, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label234;
Label331:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label332; }
else
{ goto Label389; }
Label332:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label333; }
else
{ goto Label334; }
Label333:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 11, 12, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label237;
Label334:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label335; }
else
{ goto Label360; }
Label335:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label336; }
else
{ goto Label337; }
Label336:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 13, 14, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label240;
Label337:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label338; }
else
{ goto Label349; }
Label338:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label339; }
else
{ goto Label340; }
Label339:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 15, 16, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label243;
Label340:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label341; }
else
{ goto Label346; }
Label341:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label342; }
else
{ goto Label345; }
Label342:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label344; }
Label344:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 1,0);
permutedRemoveFromDomain(PERM_ARGS, 8,0);
Label345:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label346:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label348; }
Label348:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 1,0);
Label349:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label350; }
else
{ return; }
Label350:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ return; }
else
{ goto Label354; }
Label354:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label356; }
Label356:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 1,0);
Label360:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label374; }
Label374:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label375; }
else
{ return; }
Label375:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label376; }
else
{ return; }
Label376:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ return; }
else
{ goto Label378; }
Label378:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ return; }
else
{ goto Label380; }
Label380:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 1,0);
Label389:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label390; }
else
{ goto Label393; }
Label390:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label392; }
Label392:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label360;
Label393:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ return; }
else
{ goto Label401; }
Label401:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label402; }
else
{ goto Label409; }
Label402:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label403; }
else
{ goto Label406; }
Label403:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label405; }
Label405:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
Label406:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label408; }
Label408:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
Label409:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label411; }
Label411:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
Label412:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label413; }
else
{ goto Label416; }
Label413:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label414; }
else
{ goto Label415; }
Label414:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 11, 12, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label280;
Label415:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label389;
Label416:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label417; }
else
{ goto Label420; }
Label417:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label418; }
else
{ goto Label419; }
Label418:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 13, 14, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label284;
Label419:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label393;
Label420:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label421; }
else
{ goto Label424; }
Label421:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label422; }
else
{ goto Label423; }
Label422:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 15, 16, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label288;
Label423:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 15, 16, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label401;
Label424:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label425:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label426; }
else
{ goto Label429; }
Label426:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label427; }
else
{ goto Label428; }
Label427:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 9, 10, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label299;
Label428:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 9, 10, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label412;
Label429:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label430; }
else
{ goto Label433; }
Label430:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label431; }
else
{ goto Label432; }
Label431:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 11, 12, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label303;
Label432:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 11, 12, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label416;
Label433:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label434; }
else
{ goto Label437; }
Label434:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label435; }
else
{ goto Label436; }
Label435:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 13, 14, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label307;
Label436:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 13, 14, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label420;
Label437:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label438:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label439; }
else
{ goto Label442; }
Label439:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label440; }
else
{ goto Label441; }
Label440:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 7, 8, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label312;
Label441:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 7, 8, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label425;
Label442:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label443; }
else
{ goto Label446; }
Label443:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label444; }
else
{ goto Label445; }
Label444:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 9, 10, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label316;
Label445:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 9, 10, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label429;
Label446:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label447; }
else
{ goto Label450; }
Label447:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label448; }
else
{ goto Label449; }
Label448:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 11, 12, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label320;
Label449:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 11, 12, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label433;
Label450:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label451:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label452; }
else
{ goto Label587; }
Label452:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label453; }
else
{ goto Label454; }
Label453:     (void)1;
{
const int new_perm[20] = { 1, 2, 5, 6, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label325;
Label454:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label455; }
else
{ goto Label574; }
Label455:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label456; }
else
{ goto Label457; }
Label456:     (void)1;
{
const int new_perm[20] = { 1, 2, 7, 8, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label328;
Label457:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label458; }
else
{ goto Label545; }
Label458:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label459; }
else
{ goto Label460; }
Label459:     (void)1;
{
const int new_perm[20] = { 1, 2, 9, 10, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label331;
Label460:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label461; }
else
{ goto Label494; }
Label461:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label462; }
else
{ goto Label463; }
Label462:     (void)1;
{
const int new_perm[20] = { 1, 2, 11, 12, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label334;
Label463:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label464; }
else
{ goto Label479; }
Label464:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label465; }
else
{ goto Label466; }
Label465:     (void)1;
{
const int new_perm[20] = { 1, 2, 13, 14, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label337;
Label466:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label467; }
else
{ goto Label470; }
Label467:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label468; }
else
{ goto Label469; }
Label468:     (void)1;
{
const int new_perm[20] = { 1, 2, 15, 16, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label340;
Label469:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label470:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label471; }
else
{ goto Label476; }
Label471:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label472; }
else
{ goto Label475; }
Label472:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label474; }
Label474:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 8,0);
Label475:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label476:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label478; }
Label478:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,0);
Label479:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label480; }
else
{ goto Label483; }
Label480:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label481; }
else
{ goto Label482; }
Label481:     (void)1;
{
const int new_perm[20] = { 1, 2, 15, 16, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label349;
Label482:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label470;
Label483:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ goto Label484; }
else
{ goto Label489; }
Label484:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ return; }
else
{ goto Label488; }
Label488:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,0);
Label489:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label490; }
else
{ return; }
Label490:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ return; }
else
{ goto Label492; }
Label492:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,0);
Label494:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ goto Label495; }
else
{ goto Label516; }
Label495:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label496; }
else
{ goto Label507; }
Label496:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label498; }
Label498:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label499; }
else
{ goto Label506; }
Label499:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ return; }
else
{ goto Label501; }
Label501:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label502; }
else
{ return; }
Label502:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ return; }
else
{ goto Label504; }
Label504:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label506:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label484;
Label507:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label508; }
else
{ return; }
Label508:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ return; }
else
{ goto Label510; }
Label510:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 16, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label484;
Label516:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label517; }
else
{ goto Label532; }
Label517:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label518; }
else
{ goto Label527; }
Label518:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label519; }
else
{ goto Label524; }
Label519:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label520; }
else
{ goto Label521; }
Label520:     (void)1;
{
const int new_perm[20] = { 1, 2, 13, 14, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label376;
Label521:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label522; }
else
{ goto Label523; }
Label522:     (void)1;
{
const int new_perm[20] = { 1, 2, 15, 16, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label378;
Label523:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 8,0);
Label524:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label526; }
Label526:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label490;
Label527:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label528; }
else
{ goto Label531; }
Label528:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ return; }
else
{ goto Label530; }
Label530:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
Label531:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 16, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label490;
Label532:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label533; }
else
{ goto Label540; }
Label533:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label535; }
Label535:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label536; }
else
{ return; }
Label536:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ return; }
else
{ goto Label538; }
Label538:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,0);
Label540:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label541; }
else
{ return; }
Label541:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ return; }
else
{ goto Label543; }
Label543:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
Label545:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label546; }
else
{ goto Label549; }
Label546:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label547; }
else
{ goto Label548; }
Label547:     (void)1;
{
const int new_perm[20] = { 1, 2, 11, 12, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label389;
Label548:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label494;
Label549:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label550; }
else
{ goto Label559; }
Label550:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label551; }
else
{ goto Label552; }
Label551:     (void)1;
{
const int new_perm[20] = { 1, 2, 13, 14, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label393;
Label552:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ goto Label553; }
else
{ goto Label554; }
Label553:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label507;
Label554:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label555; }
else
{ goto Label558; }
Label555:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label556; }
else
{ goto Label557; }
Label556:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label528;
Label557:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label541;
Label558:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 15, 16, 9, 10, 11, 12, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label489;
Label559:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label560; }
else
{ goto Label573; }
Label560:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label561; }
else
{ goto Label562; }
Label561:     (void)1;
{
const int new_perm[20] = { 1, 2, 15, 16, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label401;
Label562:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label563; }
else
{ goto Label570; }
Label563:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label564; }
else
{ goto Label567; }
Label564:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label566; }
Label566:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
Label567:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label569; }
Label569:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
Label570:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label572; }
Label572:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
Label573:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label574:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label575; }
else
{ goto Label578; }
Label575:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label576; }
else
{ goto Label577; }
Label576:     (void)1;
{
const int new_perm[20] = { 1, 2, 9, 10, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label412;
Label577:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 9, 10, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label545;
Label578:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label579; }
else
{ goto Label582; }
Label579:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label580; }
else
{ goto Label581; }
Label580:     (void)1;
{
const int new_perm[20] = { 1, 2, 11, 12, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label416;
Label581:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 11, 12, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label549;
Label582:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label583; }
else
{ goto Label586; }
Label583:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label584; }
else
{ goto Label585; }
Label584:     (void)1;
{
const int new_perm[20] = { 1, 2, 13, 14, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label420;
Label585:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 13, 14, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label559;
Label586:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label587:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label588; }
else
{ goto Label591; }
Label588:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label589; }
else
{ goto Label590; }
Label589:     (void)1;
{
const int new_perm[20] = { 1, 2, 7, 8, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label425;
Label590:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 7, 8, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label574;
Label591:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label592; }
else
{ goto Label595; }
Label592:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label593; }
else
{ goto Label594; }
Label593:     (void)1;
{
const int new_perm[20] = { 1, 2, 9, 10, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label429;
Label594:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 9, 10, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label578;
Label595:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label596; }
else
{ goto Label599; }
Label596:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label597; }
else
{ goto Label598; }
Label597:     (void)1;
{
const int new_perm[20] = { 1, 2, 11, 12, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label433;
Label598:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 11, 12, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label582;
Label599:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label600:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label601; }
else
{ goto Label604; }
Label601:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label602; }
else
{ goto Label603; }
Label602:     (void)1;
{
const int new_perm[20] = { 1, 2, 5, 6, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label438;
Label603:     (void)1;
{
const int new_perm[20] = { 1, 2, 5, 6, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label587;
Label604:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label605; }
else
{ goto Label608; }
Label605:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label606; }
else
{ goto Label607; }
Label606:     (void)1;
{
const int new_perm[20] = { 1, 2, 7, 8, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label442;
Label607:     (void)1;
{
const int new_perm[20] = { 1, 2, 7, 8, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label591;
Label608:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label609; }
else
{ goto Label612; }
Label609:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label610; }
else
{ goto Label611; }
Label610:     (void)1;
{
const int new_perm[20] = { 1, 2, 9, 10, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label446;
Label611:     (void)1;
{
const int new_perm[20] = { 1, 2, 9, 10, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label595;
Label612:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label613:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,0))
{ goto Label614; }
else
{ goto Label769; }
Label614:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label615; }
else
{ goto Label616; }
Label615:     (void)1;
{
const int new_perm[20] = { 3, 4, 1, 2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label451;
Label616:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label617; }
else
{ goto Label756; }
Label617:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label618; }
else
{ goto Label619; }
Label618:     (void)1;
{
const int new_perm[20] = { 5, 6, 1, 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label454;
Label619:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label620; }
else
{ goto Label729; }
Label620:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label621; }
else
{ goto Label622; }
Label621:     (void)1;
{
const int new_perm[20] = { 7, 8, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label457;
Label622:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label623; }
else
{ goto Label664; }
Label623:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label624; }
else
{ goto Label625; }
Label624:     (void)1;
{
const int new_perm[20] = { 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label460;
Label625:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label626; }
else
{ goto Label643; }
Label626:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label627; }
else
{ goto Label628; }
Label627:     (void)1;
{
const int new_perm[20] = { 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label463;
Label628:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label629; }
else
{ goto Label632; }
Label629:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label630; }
else
{ goto Label631; }
Label630:     (void)1;
{
const int new_perm[20] = { 13, 14, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label466;
Label631:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label632:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label633; }
else
{ goto Label636; }
Label633:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label634; }
else
{ goto Label635; }
Label634:     (void)1;
{
const int new_perm[20] = { 15, 16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label470;
Label635:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label636:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label637; }
else
{ goto Label642; }
Label637:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label638; }
else
{ goto Label641; }
Label638:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ return; }
else
{ goto Label640; }
Label640:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 8,0);
Label641:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label642:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,0);
Label643:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label644; }
else
{ goto Label647; }
Label644:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label645; }
else
{ goto Label646; }
Label645:     (void)1;
{
const int new_perm[20] = { 13, 14, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label479;
Label646:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label632;
Label647:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ goto Label648; }
else
{ goto Label657; }
Label648:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label649; }
else
{ goto Label656; }
Label649:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label650; }
else
{ goto Label651; }
Label650:     (void)1;
{
const int new_perm[20] = { 15, 16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label484;
Label651:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label652; }
else
{ goto Label655; }
Label652:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ return; }
else
{ goto Label654; }
Label654:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label655:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,0);
Label656:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,0);
Label657:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label658; }
else
{ return; }
Label658:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label659; }
else
{ return; }
Label659:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label660; }
else
{ goto Label661; }
Label660:     (void)1;
{
const int new_perm[20] = { 15, 16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label490;
Label661:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 8,0);
Label664:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ goto Label665; }
else
{ goto Label686; }
Label665:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label666; }
else
{ goto Label677; }
Label666:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label667; }
else
{ goto Label668; }
Label667:     (void)1;
{
const int new_perm[20] = { 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label495;
Label668:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label669; }
else
{ goto Label676; }
Label669:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label670; }
else
{ goto Label671; }
Label670:     (void)1;
{
const int new_perm[20] = { 13, 14, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label498;
Label671:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label672; }
else
{ goto Label675; }
Label672:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label673; }
else
{ goto Label674; }
Label673:     (void)1;
{
const int new_perm[20] = { 15, 16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label501;
Label674:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label675:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 9, 10, 15, 16, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label651;
Label676:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label648;
Label677:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label678; }
else
{ goto Label681; }
Label678:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label679; }
else
{ goto Label680; }
Label679:     (void)1;
{
const int new_perm[20] = { 13, 14, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label507;
Label680:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label648;
Label681:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label682; }
else
{ goto Label685; }
Label682:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ return; }
else
{ goto Label684; }
Label684:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,0);
Label685:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label686:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label687; }
else
{ goto Label712; }
Label687:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label688; }
else
{ goto Label703; }
Label688:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label689; }
else
{ goto Label698; }
Label689:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label690; }
else
{ goto Label695; }
Label690:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label691; }
else
{ goto Label692; }
Label691:     (void)1;
{
const int new_perm[20] = { 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label519;
Label692:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label693; }
else
{ goto Label694; }
Label693:     (void)1;
{
const int new_perm[20] = { 13, 14, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label521;
Label694:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 7,0);
permutedRemoveFromDomain(PERM_ARGS, 8,0);
Label695:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label696; }
else
{ goto Label697; }
Label696:     (void)1;
{
const int new_perm[20] = { 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label524;
Label697:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 9, 10, 15, 16, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label659;
Label698:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label699; }
else
{ goto Label700; }
Label699:     (void)1;
{
const int new_perm[20] = { 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label527;
Label700:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label701; }
else
{ return; }
Label701:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label659;
Label703:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label704; }
else
{ goto Label711; }
Label704:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label705; }
else
{ goto Label708; }
Label705:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label706; }
else
{ goto Label707; }
Label706:     (void)1;
{
const int new_perm[20] = { 13, 14, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label528;
Label707:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 7,1);
Label708:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label709; }
else
{ return; }
Label709:     (void)1;
{
const int new_perm[20] = { 13, 14, 1, 2, 3, 4, 5, 6, 7, 8, 15, 16, 9, 10, 11, 12, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label490;
Label711:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label700;
Label712:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label713; }
else
{ goto Label722; }
Label713:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label714; }
else
{ goto Label715; }
Label714:     (void)1;
{
const int new_perm[20] = { 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label532;
Label715:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label716; }
else
{ return; }
Label716:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label717; }
else
{ return; }
Label717:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label718; }
else
{ goto Label719; }
Label718:     (void)1;
{
const int new_perm[20] = { 13, 14, 1, 2, 3, 4, 11, 12, 7, 8, 9, 10, 5, 6, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label536;
Label719:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 7,0);
Label722:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label723; }
else
{ return; }
Label723:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label724; }
else
{ return; }
Label724:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label725; }
else
{ goto Label726; }
Label725:     (void)1;
{
const int new_perm[20] = { 13, 14, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label541;
Label726:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 7,1);
Label729:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label730; }
else
{ goto Label733; }
Label730:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label731; }
else
{ goto Label732; }
Label731:     (void)1;
{
const int new_perm[20] = { 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label545;
Label732:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 9, 10, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label664;
Label733:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label734; }
else
{ goto Label747; }
Label734:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label735; }
else
{ goto Label736; }
Label735:     (void)1;
{
const int new_perm[20] = { 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label549;
Label736:     (void)1;
if(permutedInDomain(PERM_ARGS, 9,0))
{ goto Label737; }
else
{ goto Label738; }
Label737:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 11, 12, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label677;
Label738:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label739; }
else
{ goto Label746; }
Label739:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label740; }
else
{ goto Label743; }
Label740:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label741; }
else
{ goto Label742; }
Label741:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 11, 12, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label705;
Label742:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 11, 12, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label724;
Label743:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label744; }
else
{ return; }
Label744:     (void)1;
{
const int new_perm[20] = { 13, 14, 1, 2, 3, 4, 5, 6, 11, 12, 15, 16, 7, 8, 9, 10, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label489;
Label746:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 11, 12, 13, 14, 7, 8, 9, 10, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label657;
Label747:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label748; }
else
{ goto Label755; }
Label748:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ goto Label749; }
else
{ goto Label750; }
Label749:     (void)1;
{
const int new_perm[20] = { 13, 14, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label559;
Label750:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label751; }
else
{ goto Label754; }
Label751:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,1))
{ goto Label752; }
else
{ goto Label753; }
Label752:     (void)1;
{
const int new_perm[20] = { 15, 16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label562;
Label753:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,0);
Label754:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label755:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label756:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label757; }
else
{ goto Label760; }
Label757:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label758; }
else
{ goto Label759; }
Label758:     (void)1;
{
const int new_perm[20] = { 7, 8, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label574;
Label759:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 7, 8, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label729;
Label760:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label761; }
else
{ goto Label764; }
Label761:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label762; }
else
{ goto Label763; }
Label762:     (void)1;
{
const int new_perm[20] = { 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label578;
Label763:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 9, 10, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label733;
Label764:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label765; }
else
{ goto Label768; }
Label765:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label766; }
else
{ goto Label767; }
Label766:     (void)1;
{
const int new_perm[20] = { 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label582;
Label767:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 11, 12, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label747;
Label768:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label769:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label770; }
else
{ goto Label773; }
Label770:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label771; }
else
{ goto Label772; }
Label771:     (void)1;
{
const int new_perm[20] = { 5, 6, 1, 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label587;
Label772:     (void)1;
{
const int new_perm[20] = { 1, 2, 5, 6, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label756;
Label773:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label774; }
else
{ goto Label777; }
Label774:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label775; }
else
{ goto Label776; }
Label775:     (void)1;
{
const int new_perm[20] = { 7, 8, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label591;
Label776:     (void)1;
{
const int new_perm[20] = { 1, 2, 7, 8, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label760;
Label777:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label778; }
else
{ goto Label781; }
Label778:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label779; }
else
{ goto Label780; }
Label779:     (void)1;
{
const int new_perm[20] = { 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label595;
Label780:     (void)1;
{
const int new_perm[20] = { 1, 2, 9, 10, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label764;
Label781:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label782:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,0))
{ goto Label783; }
else
{ goto Label786; }
Label783:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label784; }
else
{ goto Label785; }
Label784:     (void)1;
{
const int new_perm[20] = { 3, 4, 1, 2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label600;
Label785:     (void)1;
{
const int new_perm[20] = { 3, 4, 1, 2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label769;
Label786:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label787; }
else
{ goto Label790; }
Label787:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label788; }
else
{ goto Label789; }
Label788:     (void)1;
{
const int new_perm[20] = { 5, 6, 1, 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label604;
Label789:     (void)1;
{
const int new_perm[20] = { 5, 6, 1, 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label773;
Label790:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label791; }
else
{ goto Label794; }
Label791:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label792; }
else
{ goto Label793; }
Label792:     (void)1;
{
const int new_perm[20] = { 7, 8, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label608;
Label793:     (void)1;
{
const int new_perm[20] = { 7, 8, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label777;
Label794:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 9,1);
Label795:     (void)1;
if(permutedInDomain(PERM_ARGS, 0,0))
{ goto Label796; }
else
{ goto Label1423; }
Label796:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,0))
{ goto Label797; }
else
{ goto Label1416; }
Label797:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label798; }
else
{ goto Label1409; }
Label798:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label799; }
else
{ goto Label1402; }
Label799:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label800; }
else
{ goto Label1395; }
Label800:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label801; }
else
{ goto Label1302; }
Label801:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label802; }
else
{ goto Label1179; }
Label802:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ return; }
else
{ goto Label1014; }
Label1014:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label1015; }
else
{ return; }
Label1015:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ return; }
else
{ goto Label1073; }
Label1073:     (void)1;
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label1074; }
else
{ goto Label1115; }
Label1074:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1075; }
else
{ goto Label1106; }
Label1075:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ return; }
else
{ goto Label1097; }
Label1097:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ return; }
else
{ goto Label1099; }
Label1099:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ return; }
else
{ goto Label1101; }
Label1101:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ return; }
else
{ goto Label1103; }
Label1103:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,1))
{ return; }
else
{ goto Label1105; }
Label1105:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 1,1);
Label1106:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1107; }
else
{ goto Label1108; }
Label1107:     (void)1;
{
const int new_perm[20] = { 5, 6, 1, 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1097;
Label1108:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1109; }
else
{ goto Label1110; }
Label1109:     (void)1;
{
const int new_perm[20] = { 7, 8, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1099;
Label1110:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label1111; }
else
{ goto Label1112; }
Label1111:     (void)1;
{
const int new_perm[20] = { 9, 10, 1, 2, 7, 8, 3, 4, 5, 6, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1101;
Label1112:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ goto Label1113; }
else
{ goto Label1114; }
Label1113:     (void)1;
{
const int new_perm[20] = { 1, 2, 11, 12, 3, 4, 5, 6, 9, 10, 7, 8, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1103;
Label1114:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 6,1);
Label1115:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1116; }
else
{ goto Label1117; }
Label1116:     (void)1;
{
const int new_perm[20] = { 3, 4, 1, 2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1106;
Label1117:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1118; }
else
{ goto Label1119; }
Label1118:     (void)1;
{
const int new_perm[20] = { 5, 6, 1, 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1108;
Label1119:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1120; }
else
{ goto Label1121; }
Label1120:     (void)1;
{
const int new_perm[20] = { 7, 8, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1110;
Label1121:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label1122; }
else
{ goto Label1123; }
Label1122:     (void)1;
{
const int new_perm[20] = { 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1112;
Label1123:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 5,1);
permutedRemoveFromDomain(PERM_ARGS, 6,1);
Label1179:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label1180; }
else
{ goto Label1181; }
Label1180:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1014;
Label1181:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label1182; }
else
{ goto Label1261; }
Label1182:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label1183; }
else
{ goto Label1224; }
Label1183:     (void)1;
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label1184; }
else
{ goto Label1215; }
Label1184:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ return; }
else
{ goto Label1206; }
Label1206:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ return; }
else
{ goto Label1208; }
Label1208:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ return; }
else
{ goto Label1210; }
Label1210:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ return; }
else
{ goto Label1212; }
Label1212:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ return; }
else
{ goto Label1214; }
Label1214:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
permutedRemoveFromDomain(PERM_ARGS, 8,1);
Label1215:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1216; }
else
{ goto Label1217; }
Label1216:     (void)1;
{
const int new_perm[20] = { 3, 4, 1, 2, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1206;
Label1217:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1218; }
else
{ goto Label1219; }
Label1218:     (void)1;
{
const int new_perm[20] = { 5, 6, 1, 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1208;
Label1219:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1220; }
else
{ goto Label1221; }
Label1220:     (void)1;
{
const int new_perm[20] = { 7, 8, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1210;
Label1221:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label1222; }
else
{ goto Label1223; }
Label1222:     (void)1;
{
const int new_perm[20] = { 9, 10, 1, 2, 3, 4, 7, 8, 5, 6, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1212;
Label1223:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 5,1);
permutedRemoveFromDomain(PERM_ARGS, 8,1);
Label1224:     (void)1;
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label1225; }
else
{ goto Label1254; }
Label1225:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1226; }
else
{ goto Label1247; }
Label1226:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ return; }
else
{ goto Label1240; }
Label1240:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ return; }
else
{ goto Label1242; }
Label1242:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ return; }
else
{ goto Label1244; }
Label1244:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ return; }
else
{ goto Label1246; }
Label1246:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 1,0);
Label1247:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1248; }
else
{ goto Label1249; }
Label1248:     (void)1;
{
const int new_perm[20] = { 1, 2, 5, 6, 3, 4, 7, 8, 9, 10, 11, 12, 15, 16, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1240;
Label1249:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1250; }
else
{ goto Label1251; }
Label1250:     (void)1;
{
const int new_perm[20] = { 7, 8, 1, 2, 5, 6, 3, 4, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1242;
Label1251:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label1252; }
else
{ goto Label1253; }
Label1252:     (void)1;
{
const int new_perm[20] = { 1, 2, 9, 10, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1244;
Label1253:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,0);
permutedRemoveFromDomain(PERM_ARGS, 5,0);
Label1254:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1255; }
else
{ goto Label1256; }
Label1255:     (void)1;
{
const int new_perm[20] = { 3, 4, 1, 2, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1247;
Label1256:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1257; }
else
{ goto Label1258; }
Label1257:     (void)1;
{
const int new_perm[20] = { 5, 6, 1, 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1249;
Label1258:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1259; }
else
{ goto Label1260; }
Label1259:     (void)1;
{
const int new_perm[20] = { 7, 8, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1251;
Label1260:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 4,0);
permutedRemoveFromDomain(PERM_ARGS, 5,0);
Label1261:     (void)1;
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label1262; }
else
{ goto Label1293; }
Label1262:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ return; }
else
{ goto Label1284; }
Label1284:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ return; }
else
{ goto Label1286; }
Label1286:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ return; }
else
{ goto Label1288; }
Label1288:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ return; }
else
{ goto Label1290; }
Label1290:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,1))
{ return; }
else
{ goto Label1292; }
Label1292:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,1);
Label1293:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1294; }
else
{ goto Label1295; }
Label1294:     (void)1;
{
const int new_perm[20] = { 3, 4, 1, 2, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1284;
Label1295:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1296; }
else
{ goto Label1297; }
Label1296:     (void)1;
{
const int new_perm[20] = { 5, 6, 1, 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1286;
Label1297:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1298; }
else
{ goto Label1299; }
Label1298:     (void)1;
{
const int new_perm[20] = { 7, 8, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1288;
Label1299:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ goto Label1300; }
else
{ goto Label1301; }
Label1300:     (void)1;
{
const int new_perm[20] = { 9, 10, 1, 2, 3, 4, 7, 8, 5, 6, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1290;
Label1301:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 5,1);
Label1302:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label1303; }
else
{ goto Label1304; }
Label1303:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1179;
Label1304:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label1305; }
else
{ goto Label1306; }
Label1305:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 16, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1181;
Label1306:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,0))
{ goto Label1307; }
else
{ goto Label1366; }
Label1307:     (void)1;
if(permutedInDomain(PERM_ARGS, 8,1))
{ goto Label1308; }
else
{ goto Label1337; }
Label1308:     (void)1;
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label1309; }
else
{ goto Label1330; }
Label1309:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ return; }
else
{ goto Label1323; }
Label1323:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ return; }
else
{ goto Label1325; }
Label1325:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ return; }
else
{ goto Label1327; }
Label1327:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ return; }
else
{ goto Label1329; }
Label1329:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,0);
Label1330:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1331; }
else
{ goto Label1332; }
Label1331:     (void)1;
{
const int new_perm[20] = { 3, 4, 1, 2, 5, 6, 7, 8, 9, 10, 15, 16, 13, 14, 11, 12, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1323;
Label1332:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1333; }
else
{ goto Label1334; }
Label1333:     (void)1;
{
const int new_perm[20] = { 5, 6, 1, 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1325;
Label1334:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1335; }
else
{ goto Label1336; }
Label1335:     (void)1;
{
const int new_perm[20] = { 7, 8, 1, 2, 3, 4, 5, 6, 9, 10, 15, 16, 13, 14, 11, 12, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1327;
Label1336:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 4,0);
Label1337:     (void)1;
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label1338; }
else
{ goto Label1359; }
Label1338:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ return; }
else
{ goto Label1352; }
Label1352:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ return; }
else
{ goto Label1354; }
Label1354:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ return; }
else
{ goto Label1356; }
Label1356:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ return; }
else
{ goto Label1358; }
Label1358:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,0);
Label1359:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1360; }
else
{ goto Label1361; }
Label1360:     (void)1;
{
const int new_perm[20] = { 3, 4, 1, 2, 5, 6, 7, 8, 9, 10, 15, 16, 13, 14, 11, 12, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1352;
Label1361:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1362; }
else
{ goto Label1363; }
Label1362:     (void)1;
{
const int new_perm[20] = { 5, 6, 1, 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1354;
Label1363:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1364; }
else
{ goto Label1365; }
Label1364:     (void)1;
{
const int new_perm[20] = { 7, 8, 1, 2, 3, 4, 5, 6, 9, 10, 15, 16, 13, 14, 11, 12, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1356;
Label1365:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 4,0);
Label1366:     (void)1;
if(permutedInDomain(PERM_ARGS, 0,1))
{ goto Label1367; }
else
{ goto Label1388; }
Label1367:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ return; }
else
{ goto Label1381; }
Label1381:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ return; }
else
{ goto Label1383; }
Label1383:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ return; }
else
{ goto Label1385; }
Label1385:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,1))
{ return; }
else
{ goto Label1387; }
Label1387:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 0,0);
Label1388:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,1))
{ goto Label1389; }
else
{ goto Label1390; }
Label1389:     (void)1;
{
const int new_perm[20] = { 3, 4, 1, 2, 5, 6, 7, 8, 9, 10, 15, 16, 13, 14, 11, 12, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1381;
Label1390:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,1))
{ goto Label1391; }
else
{ goto Label1392; }
Label1391:     (void)1;
{
const int new_perm[20] = { 5, 6, 1, 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1383;
Label1392:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,1))
{ goto Label1393; }
else
{ goto Label1394; }
Label1393:     (void)1;
{
const int new_perm[20] = { 7, 8, 1, 2, 3, 4, 5, 6, 9, 10, 15, 16, 13, 14, 11, 12, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1385;
Label1394:     (void)1;
permutedRemoveFromDomain(PERM_ARGS, 4,0);
Label1395:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label1396; }
else
{ goto Label1397; }
Label1396:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1302;
Label1397:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label1398; }
else
{ goto Label1399; }
Label1398:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1304;
Label1399:     (void)1;
if(permutedInDomain(PERM_ARGS, 7,0))
{ goto Label1400; }
else
{ return; }
Label1400:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 15, 16, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1306;
Label1402:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label1403; }
else
{ goto Label1404; }
Label1403:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 9, 10, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1395;
Label1404:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label1405; }
else
{ goto Label1406; }
Label1405:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 11, 12, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1397;
Label1406:     (void)1;
if(permutedInDomain(PERM_ARGS, 6,0))
{ goto Label1407; }
else
{ return; }
Label1407:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 5, 6, 13, 14, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1399;
Label1409:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label1410; }
else
{ goto Label1411; }
Label1410:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 7, 8, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1402;
Label1411:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label1412; }
else
{ goto Label1413; }
Label1412:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 9, 10, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1404;
Label1413:     (void)1;
if(permutedInDomain(PERM_ARGS, 5,0))
{ goto Label1414; }
else
{ return; }
Label1414:     (void)1;
{
const int new_perm[20] = { 1, 2, 3, 4, 11, 12, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1406;
Label1416:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label1417; }
else
{ goto Label1418; }
Label1417:     (void)1;
{
const int new_perm[20] = { 1, 2, 5, 6, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1409;
Label1418:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label1419; }
else
{ goto Label1420; }
Label1419:     (void)1;
{
const int new_perm[20] = { 1, 2, 7, 8, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1411;
Label1420:     (void)1;
if(permutedInDomain(PERM_ARGS, 4,0))
{ goto Label1421; }
else
{ return; }
Label1421:     (void)1;
{
const int new_perm[20] = { 1, 2, 9, 10, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1413;
Label1423:     (void)1;
if(permutedInDomain(PERM_ARGS, 1,0))
{ goto Label1424; }
else
{ goto Label1425; }
Label1424:     (void)1;
{
const int new_perm[20] = { 3, 4, 1, 2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1416;
Label1425:     (void)1;
if(permutedInDomain(PERM_ARGS, 2,0))
{ goto Label1426; }
else
{ goto Label1427; }
Label1426:     (void)1;
{
const int new_perm[20] = { 5, 6, 1, 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1418;
Label1427:     (void)1;
if(permutedInDomain(PERM_ARGS, 3,0))
{ goto Label1428; }
else
{ return; }
Label1428:     (void)1;
{
const int new_perm[20] = { 7, 8, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, };
  state = applyPermutation(PERM_ARGS, new_perm);
}
goto Label1420;
}
#endif

//  Depth: 21
//  Number of nodes: 651
//  Number of nodes explored by algorithm: 1429
