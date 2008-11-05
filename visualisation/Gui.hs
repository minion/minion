module Gui where

import Control.Concurrent
import Data.List

import Graphics.UI.Gtk
import Graphics.Rendering.Cairo

import MinionParser

offset    = 3   :: Double
margin    = 20  :: Double
colWidth  = 100 :: Double
rowHeight = 100 :: Double
delay     = 300000

showSearch :: Int -> Int -> [[(Int,Domain)]] -> IO ()
showSearch width height nodeDomains = do
    let wWidth  = width * (round colWidth)
        wHeight = height * (round rowHeight)
    unsafeInitGUIForThreadedRTS
    window <- windowNew
    canvas <- drawingAreaNew
    windowSetResizable window False
    widgetSetSizeRequest window wWidth (wHeight + (round margin))
    onKeyPress window $ const (do widgetDestroy window; return True)
    onDestroy window mainQuit
    onExpose canvas $ const $ drawGrid width height canvas
    set window [containerChild := canvas]
    widgetShowAll window
    forkIO $ threadDelay delay >> showDomains (fromIntegral wWidth)
                                               (fromIntegral wHeight)
                                               canvas 0 nodeDomains
    mainGUI
    flush

drawGrid w h canvas = do
    win <- widgetGetDrawWindow canvas
    renderWithDrawable win $ grid w h
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

showDomains _ _ _  _ []                                      = return ()
showDomains width height canvas number (nodeDomains:domains) = do
    postGUIAsync $ drawDomains width height canvas nodeDomains "red"
    postGUIAsync $ treeStatus width height canvas number
    threadDelay delay
    postGUIAsync $ drawDomains width height canvas nodeDomains "black"
    showDomains width height canvas (number + 1) domains

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
