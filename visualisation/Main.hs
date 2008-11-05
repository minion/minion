module Main where

import Control.Monad
import System.Environment

import Gui
import MinionParser

main :: IO ()
main = do [width, height] <- getArgs
          input           <- getContents
          let (nodes, solutions) = parseLine $ lines input
              solAssignments     = getAssignments solutions
          showSearch (read width) (read height) $
                     getDomains [] nodes ++
                     [zip [0..((length solAssignments)-1)] solAssignments]

getDomains :: [Domain] -> [SearchNode] -> [[(Int,Domain)]]
getDomains _ []                                            = []
getDomains prevDomains ((SearchNode number domains):nodes) =
    let enumeratedDomains = zip [0..((length domains)-1)] domains
        filteredEnumeratedDomains = difference enumeratedDomains prevDomains
     in filteredEnumeratedDomains:(getDomains domains nodes)

difference :: [(Int,Domain)] -> [Domain] -> [(Int,Domain)]
difference [] _       = []
difference domains [] = domains
difference (domain:domains) (prevDomain:prevDomains)
    | (snd domain) == prevDomain = difference domains prevDomains
    | otherwise                  = domain:(difference domains prevDomains)

getAssignments :: [Solution] -> [Domain]
getAssignments []                                 = []
getAssignments ((Solution assignments):solutions) =
    (map (\x -> Domain [x]) assignments) ++ (getAssignments solutions)
