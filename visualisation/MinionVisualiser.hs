module Main where

import Control.Concurrent
import Control.Monad
import Data.List

import Graphics.UI.Gtk
import Graphics.Rendering.Cairo
import System.Environment

import MinionParser

offset    = 3   :: Double
margin    = 70  :: Double
colWidth  = 100 :: Double
rowHeight = 100 :: Double
delay     = 300000

main :: IO ()
main = do
    [numCols, numRows] <- getArgs
    let width  = (read numCols) * (round colWidth)
        height = (read numRows) * (round rowHeight)
    unsafeInitGUIForThreadedRTS
    window     <- windowNew
    set window [windowTitle := "Minion Visualiser"]
    windowSetResizable window False
    widgetSetSizeRequest window width (height + (round margin))

    box         <- vBoxNew False 0
    containerAdd window box

    toolbar     <- toolbarNew
    openFile    <- actionNew "O" "Open output file" Nothing (Just stockOpen)
    openButton  <- actionCreateToolItem openFile
    toolbarInsert toolbar (castToToolItem openButton) $ -1
    run         <- actionNew "R" "Start run" Nothing (Just stockMediaPlay)
    actionSetSensitive run False
    runButton   <- actionCreateToolItem run
    toolbarInsert toolbar (castToToolItem runButton) $ -1
    pause       <- actionNew "P" "Pause run" Nothing (Just stockMediaPause)
    actionSetSensitive pause False
    pauseButton <- actionCreateToolItem pause
    toolbarInsert toolbar (castToToolItem pauseButton) $ -1
    step        <- actionNew "S" "Step" Nothing (Just stockMediaNext)
    actionSetSensitive step False
    stepButton  <- actionCreateToolItem step
    toolbarInsert toolbar (castToToolItem stepButton) $ -1
    quit        <- actionNew "Q" "Quit" Nothing (Just stockQuit)
    quitButton  <- actionCreateToolItem quit
    toolbarInsert toolbar (castToToolItem quitButton) $ -1
    toolbarSetIconSize toolbar iconSizeSmallToolbar
    boxPackStart box toolbar PackNatural 0

    canvas      <- drawingAreaNew
    boxPackStart box canvas PackGrow 0

    pids        <- newChan
    control     <- newChan

    onActionActivate openFile $ do
        actionSetSensitive pause False
        actionSetSensitive run True
        actionSetSensitive step True
        set run [actionLabel := "Start run"]
        openFileDialog window (fromIntegral width) (fromIntegral height)
                       (read numCols) (read numRows) canvas [run,pause,step]
                       pids control
    onActionActivate run $ do writeChan control ()
                              actionSetSensitive pause True
                              actionSetSensitive run False
                              actionSetSensitive step False
    onActionActivate pause $ do writeChan control ()
                                actionSetSensitive pause False
                                actionSetSensitive run True
                                set run [actionLabel := "Resume run"]
                                actionSetSensitive step True
    onActionActivate step $ do writeChan control ()
                               writeChan control ()
                               actionSetSensitive pause False
                               actionSetSensitive run True
                               set run [actionLabel := "Resume run"]
    onActionActivate quit mainQuit
    onExpose canvas $ const $ drawGrid (read numCols) (read numRows) canvas
    onDestroy window mainQuit
    threadDelay 30000 -- some time to draw all the components

    widgetShowAll window
    mainGUI
    flush

--------------------------------------------------------------------------------
-- helper functions to assemble the list of stuff to display

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

--------------------------------------------------------------------------------
-- dialog function

openFileDialog window width height numCols numRows
               canvas buttons pids control = do
    win      <- widgetGetDrawWindow canvas
    noForked <- isEmptyChan pids
    drawWindowClear win
    case noForked of
      True  -> return () -- no previous run
      False -> readChan pids >>= killThread
    dialog   <- fileChooserDialogNew (Just "Select Minion output file")
                                     (Just window)
                                     FileChooserActionOpen
                                     [("gtk-cancel", ResponseCancel)
                                     ,("gtk-open", ResponseAccept)]
    widgetShow dialog
    response <- dialogRun dialog
    case response of 
      ResponseAccept -> do
        Just file <- fileChooserGetFilename dialog
        input     <- readFile file
        let (nodes, solutions) = parseLine $ lines input
            solAssignments     = getAssignments solutions
            nodeDomains        = getDomains [] nodes ++
                                   [zip [0..((length solAssignments)-1)]
                                        solAssignments]
        tId       <- forkIO $ readChan control >>
                              showDomains width height canvas 0 nodeDomains
                                          control buttons
        writeChan pids tId
        return ()
      _ -> mapM_ (\x -> actionSetSensitive x False) buttons
    drawGrid numCols numRows canvas
    widgetHide dialog

--------------------------------------------------------------------------------
-- draw the grid

drawGrid c r canvas = do
    win <- widgetGetDrawWindow canvas
    renderWithDrawable win $ grid c r
    return True

grid numCols numRows = do
   let width  = (fromIntegral numCols) * colWidth
       height = (fromIntegral numRows) * rowHeight
   setLineWidth 2
   drawGridLines (0,0) (0,height) (colWidth,0) (numCols + 1)
   drawGridLines (0,0) (width,0) (0,rowHeight) (numRows + 1)
   stroke

drawGridLines _ _ _ 0                       = return ()
drawGridLines (x1,y1) (x2,y2) (dx,dy) num =
    moveTo x1 y1 >>
    lineTo x2 y2 >>
    drawGridLines (x1+dx,y1+dy) (x2+dx,y2+dy) (dx,dy) (num - 1)

--------------------------------------------------------------------------------
-- show the domains and stuff

showDomains _ _ _ _ [] _ buttons                             =
    mapM_ (\x -> actionSetSensitive x False) buttons
showDomains width height canvas number (nodeDomains:domains) control b = do
    postGUIAsync $ drawDomains width height canvas nodeDomains "red"
    postGUIAsync $ treeStatus width height canvas number
    threadDelay delay
    noCommand <- isEmptyChan control
    case noCommand of
      True  -> return () -- nothing to do
      False -> readChan control >> readChan control
    postGUIAsync $ drawDomains width height canvas nodeDomains "black"
    showDomains width height canvas (number + 1) domains control b

drawDomains _ _ _ [] _                                               = return ()
drawDomains w h canvas ((cell,(Domain elements)):cellDomains) colour = do
    drawDomain w h canvas cell elements colour
    drawDomains w h canvas cellDomains colour

drawDomain w h canvas cell domain colour = do
    win             <- widgetGetDrawWindow canvas
    let cols        = round $ w / colWidth
        rows        = round $ h / rowHeight
        colNumber   = fromIntegral $ mod cell cols
        rowNumber   = fromIntegral $ div cell cols
        textX       | (length domain) > 1
                      = colWidth * colNumber + 2 * offset
                    | otherwise
                      = colWidth * colNumber + 8 * offset
        textY       | (length domain) > 1
                      = rowHeight * rowNumber + 2 * offset
                    | otherwise
                      = rowHeight * rowNumber + 4 * offset
        fontSize    | (length domain) > 1 = 12
                    | otherwise           = 48
        clearX      = round $ colWidth * colNumber + 2 * offset
        clearY      = round $ rowHeight * rowNumber + 2 * offset
        clearWidth  = round $ colWidth - 3 * offset
        clearHeight = round $ rowHeight - 3 * offset
    drawWindowClearArea win clearX clearY clearWidth clearHeight
    renderWithDrawable win $ text textX textY (colWidth - 2 * offset)
                                  fontSize colour
                                  (foldr1 (++) $
                                          intersperse ", " (map show domain))
    return True

text x y width fontSize colour text = do
    moveTo x y
    layout <- liftIO $ do
      context <- cairoCreateContext Nothing
      fontD   <- contextGetFontDescription context
      fontDescriptionSetSize fontD fontSize
      contextSetFontDescription context fontD
      layout  <- layoutEmpty context
      layoutSetMarkup layout $ markSpan [FontForeground colour] text
      layoutSetWrap layout WrapAnywhere
      layoutSetWidth layout (Just width)
      return layout
    showLayout layout

treeStatus width height canvas nodeNumber = do
    win <- widgetGetDrawWindow canvas
    drawWindowClearArea win 0             (round height)
                            (round width) (round (height + margin))
    renderWithDrawable win $ do
      moveTo offset (height + offset)
      layout <- liftIO $ do
        context <- cairoCreateContext Nothing
        layout  <- layoutText context ("Node " ++ (show nodeNumber))
        return layout
      showLayout layout
