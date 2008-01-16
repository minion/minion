del bin\*
del *.obj
cl /EHsc /O2 /Ot /GL /w /DWATCHEDLITERALS  /DNO_PRINT /DMORE_SEARCH_INFO minion\*.cpp minion\build_constraints\*.cpp /link /OUT:bin\minion-debug.exe
cl /EHsc /O2 /Ot /GL /w /DWATCHEDLITERALS  /DNO_DEBUG minion\*.cpp minion\build_constraints\*.cpp /link /OUT:bin\minion.exe
cl /EHsc /O2 /Ot /GL generators\Bibd\MinionBIBDInstanceGenerator.cpp /link /OUT:bin\bibd.exe
cl /EHsc /O2 /Ot /GL generators\Golomb\GolombMinionGenerator.cpp /link /OUT:bin\golomb.exe
cl /EHsc /O2 /Ot /GL generators\Solitaire\solitaire-solver.cpp /link /OUT:bin\solitaire.exe
cl /EHsc /O2 /Ot /GL generators\Steelmill\steelmill-solver.cpp /link /OUT: bin\steelmill.exe
cl /EHsc /O2 /Ot /GL generators\SportsSchedule\MinionSportsInstanceGenerator.cpp /link /OUT:bin\sports.exe

