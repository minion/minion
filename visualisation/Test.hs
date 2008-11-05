module Test where

import MinionParserTest

main :: IO ()
main = do putStrLn "Running parser tests..."
          runTestsMinionParser
          return ()
