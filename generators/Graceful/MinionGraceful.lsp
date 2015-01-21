(defun generate (file k p &optional 
                               (symmetry-breaking-p t)
                               (extension ".minhelp")
                               )
  (let* ((numdiffs (+ (* p k (1- k) 1/2) (* k (1- p))))
         (nodevars '())
         (diffvars '())
         (table-vector (make-absdiff-table-vector numdiffs))
                    )

  (with-open-file (str (format nil "~A-~A-~A~A" file k p extension)
                       :direction :output)

     (format str "MINION 1~%")

     (format str "\"Minion Graceful Graph Instance\"")
     (format str "\"  Version with alldiff from -Q to Q\"")
     (format str "\"K~A x P~A\"" k p)


    ;; Main variables

    ;; Nodes are just pairs of clique and index in clique
    ;; domain is 0 .. numdiffs

    (dotimes (clique p)
      (dotimes (i k)
        (push (list 'node clique i) nodevars)
        (format str "(var ~A ~A ~A ~A)~%" 
                (first nodevars)
                'discrete 
                0 
                numdiffs))
      )

    
    (format str "(alldiff ~A)~%" nodevars)

    (format str "( matrix nodematrix (~%")
    (dotimes (clique p)
      (format str "(")
      (dotimes (i k)
        (format str "(node ~A ~A)" clique i))
      (format str ")~%")
      )
    (format str "))~%")

    (format str "( print nodematrix )")

   ;; (format str "(var hack discrete ~A ~A)~%" numdiffs numdiffs)
   ;; (format str "(var minus-hack bound -~A -~A)~%" numdiffs numdiffs)

    (dotimes (clique p)
      (dotimes (i k)
        (dotimes (j i)
          (cond ((not (= i j)) 
                 (push (setup-difference str clique j clique i numdiffs 
                                         table-vector) 
                       diffvars)
                ))
          )
        (cond ((< clique (1- p))
               (push (setup-difference str clique i (1+ clique) i numdiffs 
                                       table-vector)
                     diffvars)
               )
              )
        ))

  ;;  (format str ";; Add hack variable to diffvars because it will be equal to 0~%")
    ;; (format str "(var hack discrete 0 0)")
    ;; (push 'hack diffvars)  
    (format str "(alldiff ~A)~%" diffvars)
  
    (cond (symmetry-breaking-p
            (format str "(eq (node 0 0) 0)~%")
            (format str "(occurrence (")
            (dotimes (i (1- k))
              (format str "~A " (list 'node 0 (1+ i)))
              )
            (format str "~A)" (list 'node 1 0))
            (format str "~A 1)~%" numdiffs)

            (format str "(occurrence (")
            (dotimes (i (1- k))
              (format str "~A " (list 'node 0 (1+ i)))
              )
            (format str "~A)" (list 'node 1 0))
            (format str "~A 1)~%" (1- numdiffs))

            (dotimes (i (1- k))
              (format str "(ineq ~A ~A -1)~%"
                      (list 'node 0 i)
                      (list 'node 0 (1+ i))))

            )
          )
    )
    ))

(defun setup-difference (str clique1 i clique2 j numdiffs table-vector)
  (let ((diffvar 
          (list 'diff clique1 i clique2 j))
        )
                 (format str "(var ~A ~A ~A ~A)~%"
                  diffvar
                  'discrete
                  0
                  numdiffs)

                 (format str "(table 
                         (~A ~A ~A)
                         ~A
                         )"
                         (list 'node clique1 i)
                         (list 'node clique2 j)
                         diffvar
                         table-vector)

                 diffvar
                 )
  )

(defun make-absdiff-table-vector (q)
  (let ((res nil))
  (dotimes (i (1+ q))
    (dotimes (j (1+ q))
      (cond ((= i j)
             )
            (t 
             (push (list i j (abs (- i j)))
                   res))
            )))
  (reverse res))
  )

