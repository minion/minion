

(setf *print-case* :DOWNCASE)

(defun minion-dimacs (infile &optional (outfile infile)
                             &key (inext ".cnf")
                                  (outext ".minhelp")
                                  (declare-variables t)
                                  (watched-p t)
                             &aux format variables clauses token)



  (with-open-file (sat (format nil "~A~A" infile inext)
                       :direction :input)

  (with-open-file (str (format nil "~A~A" outfile outext)
                       :direction :output)

     (do ((start (read sat t) (read sat t))
         )
      ((equal start 'p)
       (setf format (read sat t))
       (if (not (equal format 'cnf))
         (error (format nil "Sorry, do not know how to process this DIMACS format: ~A"
                        format)))
       (setf variables (read sat t))

       (setf clauses (read sat t))
       )

      (process-comments sat str start)
      )

     (format str "MINION 1~%")

     (format str "\"Minion SAT Instance\"~%")
     (format str "\"  Translated from DIMACS SAT format\"~%")
     (format str "\"  p ~A ~A ~A\"~%" 
             format variables clauses)
      
     (force-output str)
     (do ((next (read sat t) (read sat t))
          )
         ((not (equal next 'c))
          (setf token next)
          )
         (process-comments sat str next)
         )



     (let ((vartable (make-hash-table :key-type 'integer :size variables)))
       (cond (declare-variables
                 (dotimes (i variables) 
                   (declare-variable str (1+ i) vartable))
               ))

       (do (
            (count 0)
            (clause '())
            )

           ((or (equal token 'eof)
                (= count clauses))
            (cond ((equal token 'eof)
                   (format str "\"Warning: End of file before all clauses found \"~%")
                   (format str 
                           "\"  Actual number of clauses: ~A\"~%"
                           count)
                   ))
            )

         (cond ((zerop token)
                (print-clause str clause watched-p)
                (setf clause '())
                (incf count)
                )

               (t 
                 (if (not declare-variables) (declare-variable str token vartable))
                 (push token clause)
                 )
                 )

          (setf token (read sat nil 'eof))
         )))))

(defun process-comments (instr outstr character)
  (cond 
    ((member character '(#\c #\c))
     (unread-char character instr)
     (format outstr "\"~A\"~%" (read-line instr))))
)

(defun print-clause (outstr clause watched-p &aux (cl (reverse clause)))
  (format outstr (if watched-p "(watchsumgeq (" "(sumgeq ("))
  (dolist (literal cl)
    (print-literal outstr literal)
    (format outstr " ")
    )
  (format outstr ") 1)~%")
  )

(defun print-literal (outstr lit)
  (cond ((zerop lit)
         (error "Should not attempt to print literal = 0"))

        ((format outstr "v~A" lit)))) 



(defun declare-variable (outstr var hash)
  (cond ((zerop var)
         (error "Should not attempt to declare variable = 0")) 

        ((not (integerp var))
         (error (format nil 
                        "Should not attempt to declare non-integer variable = ~A"
                        var)))
        
        ((gethash (abs var) hash nil)   ;; do nothing
         )

        (t
          (setf (gethash (abs var) hash) (abs var))
          (format outstr "(var v~A bool) (var v~A bool)~%"
                  (abs var)
                  (- (abs var)))
          (format outstr "(diseq v~A v~A)~%"
                  (abs var)
                  (- (abs var)))
          )
        ))

;;; clisp stuff

(defun clisp-toplevel-sat ()
  (pprint *ARGS*)

   (cond ((null *ARGS*)
          (format *standard-output* "Usage: minion-sat -- cnffile {watched-p minhelpfile minionfile cnfext minhelpext minionext}~%")
          (format *standard-output* "~%    Note: the '--' is significant and must be entered")

          (format *standard-output* "~%      watched-p = 1 if watched sum to be used, 0 otherwise, default t")
          (format *standard-output* "~%      minhelpfile = cnffile if unspecified")
          (format *standard-output* "~%      minionfile = minhelpfile if unspecified")
          (format *standard-output* "~%        cnfext = '.cnf' if unspecified")
          (format *standard-output* "~%        minhelpext = '.minhelp' if unspecified")
          (format *standard-output* "~%        minionext = '.minion' if unspecified")

          )

         (t
           (let* ((infile (first *ARGS*))
                  (ARGS (cdr *ARGS*))
                  (watch-p (if (endp ARGS)  t (if (zerop (read-from-string (first ARGS))) nil t)))
                 (helpfile (if (endp (cdr ARGS)) infile (second ARGS)))
                 (minionfile (if (endp (cddr ARGS)) helpfile (third ARGS)))
                 (inext (if (< (length ARGS) 4) ".cnf" (fourth ARGS)))
                 (helpext (if (< (length ARGS) 5) ".minhelp" (fifth ARGS)))
                 (minionext (if (< (length ARGS) 6) ".minion" (sixth ARGS)))
                 )

           (minion-dimacs infile helpfile :inext inext :outext helpext
                          :watched-p watch-p)

           (minion-file (format nil "~A~A" helpfile helpext)
                        (format nil "~A~A" minionfile minionext))
           ))
         )

   (exit)
)





