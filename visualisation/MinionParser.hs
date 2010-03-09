module MinionParser where

import Text.ParserCombinators.Parsec

data SearchNode = SearchNode Int [Domain] -- node number, list of domains
     deriving (Show, Eq)

data Domain = Domain [Int]
     deriving (Show, Eq)

data Solution = Solution [Int] -- list of assignments
     deriving (Show, Eq)

--------------------------------------------------------------------------------

-- search tree node information with domains for all the variables
node = do
        try $ string "Node: "
        nodeNumber <- many1 digit
        char ','
        char '<'
        domains    <- many1 domain
        char '>'
        return $ SearchNode (read nodeNumber) domains

domain = do -- single element
          element <- domainElement
          return $ Domain [element]
         <|>
         do -- range
          char '['
          rangeStart <- domainElement
          rangeEnd   <- domainElement
          char ']'
          optional $ char ','
          return $ Domain [rangeStart..rangeEnd]
         <|>
         do -- set
          char '{'
          elements <- many1 domainElement
          char '}'
          optional $ char ','
          return $ Domain elements
              
domainElement = do
                 element <- many1 digit
                 optional $ char ','
                 return $ read element

-- solution with assignments for all the variables
solution = do
            try $ string "Sol: "
            assignments <- many1 assignment
            return $ Solution assignments

assignment = do
              value <- many1 digit
              optional space
              return $ read value

-- the whole lot
parseLine :: [String] -> ([SearchNode], [Solution])
parseLine []           = ([], [])
parseLine (line:lines) =
        do
          let (nn, ss) = parseLine lines
          case parse node "" line of
            Right n  -> (n:nn, ss)
            Left err -> do
                          case parse solution "" line of
                            Right s  -> (nn, s:ss)
                            Left err -> (nn, ss) -- TODO: error handling
