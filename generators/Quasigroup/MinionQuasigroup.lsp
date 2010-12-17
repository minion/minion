
(defconstant *quasigroup-kinds* '(3 7))

(setf *print-case* :UPCASE)

(defun minion-quasigroup (file kind n
                               &key 
                               (idempotent-p t) 
                               (symmetry-breaking-p t)
                               (extension ".minhelp")
                               (inverse-p nil)
                               (alldiff-p t)
                               (watchelement-p t)
                               &aux (element (if watchelement-p "watchelement" "element"))
                                    (fullfilename (format nil "~A-~A-~A-~A" file element kind n))
                               )

  (if (not (member kind *quasigroup-kinds*))
      (error "Sorry do not know about this kind of quasigroup"))

  (with-open-file (str (format nil "~A~A" fullfilename extension) :direction :output)

     (format str "MINION 1~%")

              (format str "\"Minion Quasigroup Instance\"")
              (format str "\"Kind: QG~A Size: ~A\"" kind n)
              (format str (if idempotent-p "\"Idempotent\"" "\"Not idempotent\""))
              (format str "\"CSPLib Problem prob003\"")
              (format str "\"   http://www.csplib.org\"")

    ;; Main variables
    ;; Index (i j) indicates jth entry in row i, i.e. i*j

    (dotimes (i n)
      (dotimes (j n)
        (format str "(var ~A ~A ~A ~A)~%" 
                (list 'v i j) 
                'discrete 0 (1- n)))
      )


    ;; set up main matrix

    (format str "\"In main matrix,  Row(a)[b] = c  means  a*b=c\"~%")
    (format str "(matrix quasigroup ~%  (")
      (dotimes (i n)
        (format str "   (")
        (dotimes (j n) 
          (format str "~A" (list 'v i j))
          )
        (format str ")~%")
        )
    (format str "  ))~%")
    
    ;; set up inverse matrix

    (cond (inverse-p

            (dotimes (i n)
              (dotimes (j n)
                (format str "(var ~A ~A ~A ~A)~%" 
                        (list 'inv i j) 
                        'discrete 0 (1- n)))
              )

            (format str "(matrix inverse ~%  (")
              (dotimes (i n)
                (format str "   (")
                (dotimes (j n) 
                  (format str "~A" (list 'inv i j))
                  )
                (format str ")~%")
                )
            (format str "  ))~%")

        ; link original and inverse matrix 
            (format str "\"In inverse matrix,  Row(a)[b] = c  means  a*c=b\"~%")
            (format str "\"               i.e. Row(a)[c] = b  means  a*b=c\"~%")
            (format str "\"In main matrix,     Row(a)[b] = c  means  a*b=c\"~%")
            (format str "\"               i.e. Row(a)[c] = b  means  a*c=b\"~%")

            (format str "\"Link Main and Inverse as follows\"~%")
            (format str "\"  Element(InverseRow(a),Main(a,b),b)\"~%")
            (format str "\"  Element(QuasigroupRow(a),Inverse(a,b),b)\"~%")

            (dotimes (a n)
               (dotimes (b n)
                    (format str "(~A (row inverse ~A) ~A ~A)~%" element
                            a (list 'v a b) b)
                    (format str "(~A (row quasigroup ~A) ~A ~A)~%" element
                            a (list 'inv a b) b)
                    ))
            )
          )
         


    ;;;

    (cond (alldiff-p
           (dotimes (i n)
             (format str "(alldiff (col quasigroup ~A))~%" i)
             (format str "(alldiff (row quasigroup ~A))~%" i)
           ))
          )

    ;;; idempotency

    (if idempotent-p (format str "\"Idempotency Constraints\""))

    (if idempotent-p 
      (dotimes (i n) 
        (format str "(eq (v ~A ~A) ~A)~%" i i i)))

    (case kind
      ( (3)         ;; QG3 is:  (a*b)*(b*a) = a
                    ;; Let c = b*a 
                    ;; Led d = a*b

                    ;;     b*a is the a'th entry in row b
                    ;;     a*b is the b'th entry in row a

                    ;; Then we want d*c = a
                    ;;     d*c is the c'th entry in row d
       (let ((table-vector (make-index-table-vector n)))

        ;; set up flat versin of matrix
        (format str "\"In flattened matrix,  V[a*n+b] = c  means  a*b=c\"~%")
        (format str "(vector flat-quasigroup ~%  (")
        (dotimes (i n)
            (dotimes (j n) (format str "~A" (list 'v i j)))
            )
        (format str "  ))~%")

        ;; (format str "\"Result variables are (b*a)*b and should be = a*(b*a)\"")

        (dotimes (a n)
          (dotimes (b n)
            ;; V[d*n + c] = a
            (format str "(var ~A discrete 0 ~A)~%"
                    (list 'index a b) (1- (* n n)))
            ;; (format str "(weightedsumleq (~A 1) (~A ~A) ~A)" n (list 'v a b) (list 'v b a) (list 'index a b))
            ;;(format str "(weightedsumgeq (~A 1) (~A ~A) ~A)" n (list 'v a b) (list 'v b a) (list 'index a b))
            (format str "(table (~A ~A ~A)
                                ~A
                                )"
                        (list 'v a b) (list 'v b a) (list 'index a b)
                        table-vector)
            (format str "(~A flat-quasigroup ~A ~A)~%" 
                    element
                    (list 'index a b)
                    (list 'v a b))
                    b (list 'v b a) (list 'result b a))
            )
        ))

      ( (7)         ;; QG7 is:  (b*a)*b = a*(b*a)
                    ;; Let c = b*a 
                    ;;     b*a is the a'th entry in row b
                    ;; Then we want c*b = a*c
                    ;;     c*b is the b'th entry in row c
                    ;;         = c'th entry in column b
                    ;;     a*c is the c'th entry in Row a
        (format str "\"Result variables are (b*a)*b and should be = a*(b*a)\"")
        (dotimes (a n)
          (dotimes (b n)
            (format str "(var ~A discrete 0 ~A)~%"
                    (list 'result b a) (1- n))
            (format str "(~A (col quasigroup ~A) ~A ~A)~%" element
                    b (list 'v b a) (list 'result b a))
            (format str "(~A (row quasigroup ~A) ~A ~A)~%" element
                    a (list 'v b a) (list 'result b a))
            ))
        )

      ( otherwise (error "How on earth did I get here"))
      )

    (cond (nil 
            (dotimes (i n) 
              (dotimes (j n)
                (format str "(var ~A discrete ~A ~A)"
                        (list 'location-of i j)     ;; location of i in row j
                        0 (1- n))))
    (format str "(matrix location ~%  (")
      (dotimes (i n)
        (format str "   (")
        (dotimes (j n) 
          (format str "~A" (list 'location-of i j))
          )
        (format str ")~%")
        )
    (format str "  ))~%")

            (dotimes (i n)
              (format str "(alldiff (row location ~A))~%" i)
              (dotimes (j n)
                (format str "(~A (row quasigroup ~A) ~A ~A)" element
                        j
                        (list 'location-of i j)
                        i
                        )
                ))
            ))
             
    ;; break symmetry 
    ;; common constraint is a*m >= a-1
    ;; i.e. in our terms a*(n-1) >= a-2
    ;;      in minion    a <= a*(n-1) + 2

    (cond (symmetry-breaking-p
           (format str "\"Symmetry Breaking Constraints\"")
           (dotimes (i n)
            (format str "(ineq ~A ~A ~A)~%"
                    i 
                    (list 'v i (1- n))
                    2))
           (format str "\"Var ordering should be linked to symmetry constraints\"~%")
           (format str "(var-order ")
           (dotimes (i n)
             (format str "~A" (list 'v (- n 1 i) (1- n))))
           (format str "~%")
             (dotimes (i n)
               (dotimes (j (1- n))
                 (format str "~A" (list 'v i j))
               ))
           (format str ")~%")
           ))

    (format str "( print quasigroup )")

    (format str "~%minion-end~%")

    fullfilename

    ))

;;; table vector


(defun make-index-table-vector (n)
  (let ((res nil))
  (dotimes (i n)
    (dotimes (j n)
      (cond 
            (t
             (push (list i j (+ (* i n) j))
                   res))
            )))
  (reverse res))
  )


;;; clisp stuff

(defun clisp-toplevel-quasigroup ()

   (cond ((< (length *ARGS*) 3)
          (format *standard-output* "Usage: minion-quasigroup -- filename kind n {watched minhelpext minionext}~%")
          (format *standard-output* "~%    Note: the '--' is significant and must be entered")

          (format *standard-output* "~%      filename: to which extensions will be added")
          (format *standard-output* "~%      kind: type of quasigroup, e.g. 3 or 7")
          (format *standard-output* "~%      n: size of quasigroup")
          (format *standard-output* "~%        watched = t if unspecified (uses watched literal element")
          (format *standard-output* "~%        minhelpext = '.minhelp' if unspecified")
          (format *standard-output* "~%        minionext = '.minion' if unspecified")

          )

         (t
           (let* ((filename (first *ARGS*))
                 (kind (read-from-string (second *ARGS*)))
                 (n (read-from-string (third *ARGS*)))
                 (watched-p (if (< (length *ARGS*) 4) t (read-from-string (fourth *ARGS*))))
                 (helpext (if (< (length *ARGS*) 5) ".minhelp" (fifth *ARGS*)))
                 (minionext (if (< (length *ARGS*) 6) ".minion" (sixth *ARGS*)))
                 )

           (let ((fullname (minion-quasigroup filename kind n :watchelement-p watched-p :extension helpext)))

           (minion-file (format nil "~A~A" fullname helpext)
                        (format nil "~A~A" fullname minionext))
           )))
         )

   (exit)
)






