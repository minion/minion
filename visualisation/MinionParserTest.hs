module MinionParserTest where

import Text.ParserCombinators.Parsec
import Text.ParserCombinators.Parsec.Error
import Test.HUnit

import MinionParser

--------------------------------------------------------------------------------
-- adding instance declarations for Eq to be able to test the parsers
instance Eq ParseError where
   a == b = errorMessages a == errorMessages b

instance Eq Message where
   (==) = messageEq
--------------------------------------------------------------------------------

runTestsMinionParser = runTestTT $ test
  [ "node single domains" ~:
       parse node "error" "Node: 13,<1,2,3>" ~?=
         Right (SearchNode 13 [Domain [1],Domain [2],Domain [3]])
  , "node range domains" ~:
       parse node "error" "Node: 13,<[1,3]>" ~?=
         Right (SearchNode 13 [Domain [1,2,3]])
  , "node list domains" ~:
       parse node "error" "Node: 13,<{1,3}>" ~?=
         Right (SearchNode 13 [Domain [1,3]])
  , "node mixed domains" ~:
       parse node "error" "Node: 13,<1,2,[1,3],5,6,{1,7}>" ~?=
         Right (SearchNode 13 [Domain [1],Domain [2],Domain [1,2,3],
				 Domain [5],Domain [6], Domain [1,7]])
  , "solution single value" ~:
       parse solution "error" "Sol: 13" ~?= Right (Solution [13])
  , "solution multiple values" ~:
       parse solution "error" "Sol: 13 3 4" ~?= Right (Solution [13,3,4])
  , "search no solution" ~:
       parseLine ["Node: 12,<1,2>"] ~?=
         ([(SearchNode 12 [Domain [1],Domain [2]])], [])
  , "search one solution" ~:
       parseLine ["Node: 12,<1,2>", "Sol: 1 2"] ~?=
         ([(SearchNode 12 [Domain [1],Domain [2]])], [(Solution [1,2])])
--  , "search one solution multiple lines" ~:
--       parseLine ["Node: 12,<1,2>", "Sol: 1 2", "Sol: 3 4"] ~?=
--         ([(SearchNode 12 [Domain [1],Domain [2]])], [(Solution [1,2,3,4])])
  , "search comment before" ~:
       parseLine ["Setup time: 0.0", "Node: 12,<1,2>", "Sol: 1 2"] ~?=
         ([(SearchNode 12 [Domain [1],Domain [2]])], [(Solution [1,2])])
  , "search comment after" ~:
       parseLine ["Node: 12,<1,2>", "Sol: 1 2", "Setup time: 0.0"] ~?=
         ([(SearchNode 12 [Domain [1],Domain [2]])], [(Solution [1,2])])
  ]
