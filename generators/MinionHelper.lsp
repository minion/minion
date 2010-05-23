
;; File MinionHelper.lsp 
;; Ian Gent, March 2006

;; To run, if you have clisp...

;; > clisp 
;; > (load "MinionHelper.lsp")

;; > (minion-file "myinputfile.whatever" "myoutputfile.minion")

;; 

;; Takes a more human writable format into Minion input format
;;   Also intended to make it easier to write programs to generate Minion instances
;;   Takes care of a lot of the bookkeeping, especially assigning numbers to variables

;; Main function you want to call is 

;  (minion-file "input-filename.minhelp" "output-filename.minion")

; Almost everything is in Lisp list format, which is possibly not ideal, sorry.   

; Main things to know are ... 

; 

; (var ID TYPE {data}*)     where data declares what we need to know (range or values in domain)
; 
;       declares var of the right type.  It has to be declared before it is used.   
;       ID is internal to the file and will not appear in the .minion file produced
;       Anywhere else in the .minhelp file we use ID to refer to this variable
;       including in matrices, vectors, constraints, etc.
;       ID can be ANY lisp readable term.   E.g. a list, an atom, etc.   It is allowed but 
;           not advised to use integers as they could get confused with constants
;       ID's are not typed so make sure that they are separate for matrices, vectors, vars, etc.
;       ID's are renumbered by the program to make your life easier.

; (matrix ID ( .... ) ) 
; (vector ID ( .... ) )
; (tensor ID ( .... ) )

; (print MATRIX-ID )    ;; or actually give the matrix here for convenience, 

; (objective none/minimising/maximising {VAR-ID})

; (var-order var-id1 var-id2 ... ) 

; (val-order a d d a ... )
;;      defaults to all a
;;      if length is shorter than var-order, last value is used for all 
;;              later vars
;;          e.g. a d a d --> a d a d d d d d d d ... 

; (constraint-name arg1 arg2 ... ) 

; "comments go in strings but should not break lines"   

; minion-end            ;; at end of instance 
;    --- optional if instance ends at endoffile 



(format *standard-output* ";Welcome to Minion Input Helper")
(if (equal (lisp-implementation-type) "CLISP")
    (format *standard-output* "~%; (clisp-make-executable) to make executable~%"))


(defconstant +minion-url+ "http://sourceforge.net/projects/minion")
(defconstant +minion-comment-char+ "#")

(defconstant +minion-input-version-list+ '(1 2))
(defconstant +minion-input-version+ 1)
(defconstant +minion-constraints+ 
             '(alldiff diseq eq element ineq lexleq lexless max min
                       occurrence product watchsumgeq watchsumleq sumgeq sumleq watchtable watchelement
                       weightedsumgeq weightedsumleq
                       reify reifyimplies
                       table))
                       
                       

(defconstant +minion-val-terms+ '(a d))

(defconstant +minion-var-types+
             '(bool bound sparse-bound discrete sparse-discrete))

(defconstant +minion-data-types+
             '(vector matrix tensor))

(defstruct minion
  (var-table (make-hash-table :test #'equalp))
  (type-table (make-hash-table :test #'equalp))
  (data-table (make-hash-table :test #'equalp))
  (vars-info '())
  (numdata 0)
  (comments '())
  (objective 'none)
  (constraints '())
  objective-var
  (print 'all)
  (var-order '())
  (val-order '())
  (header (format nil "MINION ~A" +minion-input-version+))
  (vars-numbered nil)
  (filename nil)
)

(defstruct minion-not
  var)


(defstruct constraint
  name
  arguments)

(defstruct matrix-col 
  matrix
  col)

(defstruct matrix-row 
  matrix
  row)

(defstruct datum
  id 
  type
  value
  unique-number ;; number of entry in table
  number
  )

(defstruct mvar
  type
  type-data
  (id nil)
  number
  sequence-number
  )

(defun mvar-string (var &optional (str nil)) 
  (format str "x~A" (mvar-number var))
  )


(defun minion-add-comment (inst comment) 
  (if (listp (minion-comments inst))
      (push comment (minion-comments inst))
      (error "Cannot add comments here because comments are not a list, sorry")
      ))

(defun minion-matrix-p (thing)
  (and (datum-p thing)
       (equal (datum-type thing) 'matrix)))

(defun minion-vector-p (thing)
  (and (datum-p thing)
       (equal (datum-type thing) 'vector)))

(defun minion-tensor-p (thing)
  (and (datum-p thing)
       (equal (datum-type thing) 'tensor)))

(defun minion-var (inst id)
  (gethash id (minion-var-table inst) nil))

(defun minion-var-number (inst id)
  (mvar-number (minion-var inst id)))

(defun minion-datum (inst type id)
  (cond ((not (member type +minion-data-types+))
         (error "type not a member of +minion-data-types+ constant")
         ))
  (gethash id 
           (gethash type (minion-data-table inst) (make-hash-table))
           nil)
  )

(defun minion-datum-number (inst type id)
  (datum-number (minion-datum inst type id)))

(defun minion-get-id (inst id)
  (cond ((minion-var inst id)) 

        (t 
          (do ((res nil) 
               (types +minion-data-types+ (cdr types))
               )
              ((or res (endp types)) 
               res)
              (setf res (minion-datum inst (car types) id))))))
        
  

(defun minion-data-list (inst data-type)
  (cond ((not (member data-type +minion-data-types+))
         (error "var type not a member of +minion-var-types+ constant")
         ))
  (let ((res '()))
       (maphash #'(lambda (x y) (declare (ignore x)) (push y res))
                (gethash data-type (minion-data-table inst) 
                         (make-hash-table)))
       (sort res #'< :key #'datum-number)))

(defun minion-var-list (inst)
  (let ((res '()))
       (maphash #'(lambda (x y) (declare (ignore x))
                    (push y res)) (minion-var-table inst) )
       (sort res #'< :key #'mvar-number)))



(defun minion-add-datum (inst data-type value 
                              &key (id nil)
                                   (error-if-present-p nil))

  (cond ((not (member data-type +minion-data-types+))
         (error "type not a member of +minion-data-types+  constant")
         ))

  (let* ((type-table (minion-data-table inst))
         (data-table (gethash data-type type-table nil))
         current-datum
         new-datum
         )

         (cond ((not data-table) 
                (setf data-table 
                      (make-hash-table :test #'equalp))
                (setf (gethash data-type type-table)
                      data-table))
               )

         (setf current-datum (if id (gethash id data-table nil) nil))

         (cond (current-datum 

                (cond 
                  (error-if-present-p 
                   (error "Datum not meant to be present when this called")
                  )

                  ((or (not (equalp data-type (datum-type current-datum))) 
                       (not (equalp value (datum-value current-datum))))
                   (error "Datum incompatible with already entered value"))

                  (t
                    current-datum ;; but nothing to add so return it
                    )
                  ))

                (t                  ;; actually add it now
                 (setf new-datum 
                       (make-datum :id (if id id (minion-numdata inst))
                                   :type data-type 
                                   :value value
                                   :unique-number (minion-numdata inst)
                                   :number (hash-table-count data-table)))
                 (incf (minion-numdata inst))
                 (setf (gethash (datum-id new-datum) data-table) 
                       new-datum)
                 new-datum
                 )
                ))
  )

        

(defun minion-add-var (inst id type type-data &key (error-if-present-p nil))
  (let* ((table (minion-var-table inst))
         (type-table (minion-type-table inst))
         (current-var (gethash id table nil))
         (sorted-type-data 
           (cond ((member type '(bound discrete sparse-bound sparse-discrete))
                  (sort type-data #'<))
                 (t 
                   type-data)))
         (new-var (make-mvar :id id :type type 
                             :type-data sorted-type-data
                            :sequence-number (hash-table-count table)))
        )
    (cond ((not (member type +minion-var-types+))
           (error "var type not a member of +minion-var-types+ constant")
           )

          ((and error-if-present-p current-var)
           (error "Variable not meant to be present when minion-add-var called")
           )
          
          ((and current-var
                (or (not (equalp type (mvar-type current-var)))
                    (not (equalp sorted-type-data (mvar-type-data current-var)))))
           (error "Var incompatible with already entered var"))

          (current-var  ;; ok 
            current-var ;; but nothing to add
            )

          (t            ;; really add var

            (setf (gethash id table) new-var)
            (setf (minion-vars-numbered inst) nil)

            (if (not (gethash type type-table nil))
                (setf (gethash type type-table)
                      (make-hash-table :test #'equalp)))

            (let ((data-table (gethash type type-table nil)
                              ))
                (push new-var 
                      (gethash type-data data-table '()))
                new-var)
          )))
  )

(defun minion-add-constraint (inst name arguments)

  (cond ((not (member name +minion-constraints+))
         (format *standard-output*
                 "Known list of constraints:~%~A~%"
                 +minion-constraints+
                 )
         (error (format nil 
                        "constraint not a member of +minion-constraints+ list: ~A"
                        name))
         ))


  (push (make-constraint :name name :arguments arguments)
        (minion-constraints inst)))


;; Call this before finalising/printing

(defun minion-number-vars (inst)
  (let* (  ; (table (minion-var-table inst))
         (type-table (minion-type-table inst))
         ; (var-counts '())
         ; (var-first '())
         (count 0)
         )

    (setf (minion-vars-info inst) '())

    (dolist (var-type +minion-var-types+)
      (let ((table (gethash var-type type-table (make-hash-table))) 
            (num-vars-type 0)
            (var-info '() )
           )
        (let (

         (work-entry 
          #'(lambda 
              (type-data varlist)

          (mapcar #'(lambda (var)
                     (setf (mvar-number var) count)
                     (incf count)
                     (incf num-vars-type)
                     )
                  (sort varlist #'< :key #'mvar-sequence-number))
          (setf var-info (append var-info
                                 (list (list type-data (length varlist)))))
          ))
        )

        (maphash work-entry table)
        (push (list var-type num-vars-type) var-info)
        (setf (minion-vars-info inst) (append (minion-vars-info inst) 
                                              (list var-info)))
        )))
    )
  (setf (minion-vars-numbered inst) t)
  )


;;;; Printing Functions

(defun date-string (&optional (time (multiple-value-list (get-decoded-time))))
	     (format nil "~A ~A ~A"
		   (fourth time) 
		   (nth (1- (fifth time))
		       '("Jan" "Feb" "Mar" "Apr" "May" "Jun" 
		         "Jul" "Aug" "Sep" "Oct" "Nov" "Dec") )
	           (sixth time)
	     ))

(defun date-time-string ()
	(let ((time (multiple-value-list (get-decoded-time))))
	     (format nil "~2,'0D:~2,'0D on ~A"
		   (third time) 
                   (second time)
                   (date-string time)
	     )))

(defun minion-datum-string (datum &optional (str nil))
      (format str "~A~A" 
              (case (datum-type datum)
                    ((vector) "v") ((matrix) "m")((tensor "t"))
                    (otherwise (error "wrong type I can't deal with")))
              (datum-number datum)))

(defun minion-print-object (inst item &optional (str *standard-output*))
;; will not work for str = nil to return a string
  (cond ((numberp item)
         (format str "~A" item))
        
        ((minion-not-p item)
         (format str "n")
         (minion-print-object inst (minion-not-var item) str)
         )
        
        ((mvar-p item)
         (mvar-string item str))

        ((constraint-p item)
         (constraint-print inst item str))

        ((datum-p item)     ;; should not be a constraint
         (minion-datum-string item str))

        ((matrix-col-p item)
         (format str "col(~A,~A)" 
                 (minion-datum-string (matrix-col-matrix item))
                 (matrix-col-col item)))

        ((matrix-row-p item)
         (format str "row(~A,~A)" 
                 (minion-datum-string (matrix-row-matrix item))
                 (matrix-row-row item)))

        ((stringp item)     ;; e.g. "a" or "d"
         (format str "~A" item))

        ((member item +minion-val-terms+)
         (format str "~A" item))

        (t                  ;; will assume it is a literal vector and recurse
         (minion-print-objects inst item str))
   )
  )

(defun minion-print-objects (inst objects 
                                  &optional (str *standard-output*) 
                                            (brackets '("[" "]"))
                                            (separator ","))
         (format str (first brackets))
         (do ((items objects (cdr items)))
             ((endp items)
              (format str "~A~%" (second brackets)))
             (minion-print-object inst (car items) str)
             (if (not (endp (cdr items)))
                 (format str "~A" separator))
             )
         )

(defun tuple-<= (a b) 
  (cond ((endp a) t)
        ((endp b) nil)
        ((< (car a) (car b)) t)
        ((= (car a) (car b))
         (tuple-<= (cdr a) (cdr b)))
        (t nil)
        ))

(defun minion-print-constraint (inst c-name arguments 
                                     &optional (str *standard-output*))
  (format str "~A" c-name)
  (cond ((equal c-name 'table)
         (format str "(~%")
         (minion-print-object inst (first arguments) str)
         (format str ",{")

         (do ((vectors (sort (second arguments) #'tuple-<=) 
                       (cdr vectors))
              )
             ((endp vectors)
              )
             (minion-print-objects inst (first vectors) str '("<" ">"))
             (if (not (endp (cdr vectors)))
               (format str ",")
               )
             )

         (format str "})~%")
         )
        
        (t 
          (minion-print-objects inst arguments str '("(" ")")))
        ))

(defun constraint-print (inst constraint &optional (str *standard-output*))
  (minion-print-constraint inst 
                           (constraint-name constraint)
                           (constraint-arguments constraint)
                           str))

(defun minion-default-val-order (vars values &optional (default "a")) 
  "last value in list is used for default if list runs out"
  (cond ((endp vars)
         ())

        ((endp values)
         (make-list (length vars) :initial-element default))

        (t
          (cons (car values)
                (minion-default-val-order 
                  (cdr vars) 
                  (cdr values) 
                  (car values)))))
  )

(defun minion-process-val-order (inst search-vars 
                                      &aux (order (minion-val-order inst)))
  (cond ((null order)
         (minion-default-val-order search-vars ()))

        ((not (listp order))
         (minion-default-val-order search-vars () order))

        ((listp order)
         (minion-default-val-order search-vars order (first order)))
        )
  )



(defun minion-output (inst &optional (str *standard-output*))

  (setq *print-case* :DOWNCASE)     ;; so that atoms like 'alldiff printed correctly

  (if (not (minion-vars-numbered inst))
      (minion-number-vars inst))

  (format str "~A~%" (minion-header inst))

          (cond ((listp (minion-comments inst))
                 (dolist (comm (reverse (minion-comments inst)))
                   (format str "~A ~A~%"
                           +minion-comment-char+ 
                           comm)))
                (t
                   (format str "~A ~A~%"
                           +minion-comment-char+ 
                           (minion-comments inst))))

  (format str "~A~%~A Minion Constraint Solver Input~%~A    ~A~%"
          +minion-comment-char+ +minion-comment-char+ +minion-comment-char+
          +minion-url+
          )
  (format str "~A Instance created using CLisp generator written by Ian Gent~%" 
          +minion-comment-char+
          )
  (format str "~A Instance created at ~A~%" 
          +minion-comment-char+
          (date-time-string))

  (dolist (info (minion-vars-info inst))
    (format str "~A~%" (second (first info))) ;; num vars of this type
    (dolist (varinfo (cdr info))
      (case (first (first info))
         
        ( (bool) )  ;; do nothing
        ( (bound discrete) 
                  (format str "~A ~A " 
                          (reduce #'min (first varinfo))
                          (reduce #'max (first varinfo)))
                  (format str "~A~%" (second varinfo)))
        ( (sparse-bound sparse-discrete) 
                  (format str "{" )
                  (do ((elts (sort (first varinfo) #'<) (cdr elts)))
                      ((endp elts)
                       (format str "} ~A~%" (second varinfo)))
                      (format str "~A" (first elts))
                      (if (cdr elts) (format str ","))
                      )
                  )
        ( otherwise
          (error "unknown variable type"))
      )
      ))

  (let* ((search-vars (if (and (minion-var-order inst)
                              (listp (minion-var-order inst)))
                         (minion-var-order inst)
                         (minion-var-list inst)))
         (search-vals (minion-process-val-order inst search-vars))
         )

        (minion-print-objects inst search-vars str)
        (minion-print-objects inst search-vals str)
        )

  (dolist (datatype +minion-data-types+)
    (let ((data (minion-data-list inst datatype))) 
         (format str "~A~%" (length data))
         (dolist (datum data)
           (minion-print-objects inst (datum-value datum) str)
           )
         ))

  (format str "objective ~A" (minion-objective inst))
  (cond ((mvar-p (minion-objective-var inst))
         (format str " ")
         (minion-print-object inst (minion-objective-var inst) str))
        )
  (format str "~%")

  (format str "print ~A~%" 
              (if (minion-matrix-p (minion-print inst))
                  (minion-print-object inst (minion-print inst) nil)
                  "none"))

  ;; constraints coming

  (minion-print-objects inst (reverse (minion-constraints inst)) str '("" "") (format nil "~%"))

  )

(defun minion-printfile (inst filename)
  (with-open-file (s filename :direction :output)
    (minion-output inst s)))




(defun minion-recursive-process-items (inst items)
  (let ((lookup (minion-get-id inst items)))

    (cond (lookup)  ;; if we have found something, return it

          ((integerp items)
           items)

          ((and (listp items) items
                (equalp (first items) 'col))
           (make-matrix-col 
                :matrix (minion-get-id inst (second items))
                                    ;; allow wrong type here for ease of programming
                :col (third items)))

          ((and (listp items) items
                (equalp (first items) 'row))
           (make-matrix-row 
                :matrix (minion-get-id inst (second items))
                                    ;; allow wrong type here for ease of programming
                :row (third items)))

          ((and (listp items)       ;; subconstraint in e.g. reify items
                items
                (member (first items) +minion-constraints+))

           (make-constraint :name (first items)
                            :arguments 
                            (minion-recursive-process-items 
                              inst (cdr items)))
           )
           

          ((and (listp items)
                items
                (equal (first items) 'not)
                )
           (make-minion-not :var (minion-get-id inst (second items)))       
            ;; must be one item, in fact bool var
           )

          ((listp items)    ;; recurse
           
           (mapcar #'(lambda (x) 
                       (minion-recursive-process-items inst x))
                   items))

          (t
            (error 
             (format nil "Cannot recognise: ~A" items)))
          )
    )
  )





;;; File Reading Stuff

(defun avoid-integer (id warning-stream)
  (if (integerp id)
      (format warning-stream 
              "WARNING: Danger in using integers as identifier: ~A~%"
              id))
  id)

(defun minion-toplevel-item-process (inst item &optional (stream nil)
                                          (warning-stream *standard-output*))
  (declare (ignore stream))
  (defun ignore-input ()
     (format warning-stream 
             "Warning - ignoring input: ~A~%"
             item))
    
  (cond 
    ((stringp item)
     (minion-add-comment inst item))

    ((or (not item)
         (not (listp item)))
     (ignore-input))

    ((equal (first item) 'filename)
     (if (< (length item) 2) (ignore-input))
     (setf (minion-filename inst) (second item))
     )

    ((equal (first item) 'var)
     (if (< (length item) 3) (ignore-input))
     (minion-add-var inst (avoid-integer (second item) warning-stream)
                     (third item) (cdddr item)))

    ((member (first item) +minion-data-types+)
     (if (< (length item) 3) (ignore-input))
     (minion-add-datum inst (first item) 
                       (minion-recursive-process-items inst (third item))
                       :id (avoid-integer (second item) warning-stream))
     )

    ((equal (first item) 'var-order)
     (setf (minion-var-order inst) 
           (minion-recursive-process-items inst (cdr item)))
     )

    ((equal (first item) 'val-order)
     (setf (minion-val-order inst) (cdr item))
     )

    ((equal (first item) 'objective)
     (if (< (length item) 2) (ignore-input))

     (setf (minion-objective inst) (second item))
     
     (cond ((equal (second item) 'none)
            t)

           (t 
            (setf (minion-objective-var inst) 
                  (minion-var inst (third item)))
            )
           ))

    ((equal (first item) 'print)
     (if (< (length item) 2) (ignore-input))

     (cond      ;;; allow a literal matrix here to make life easy
        ((and (second item) (listp (second item))
                 (equal (first (second item)) 'matrix))  

         (setf 
           (minion-print inst)
           (minion-add-datum inst 
                           (first (second item))
                           (minion-recursive-process-items 
                             inst 
                             (if (> (length (second item)) 2)
                               (third (second item))
                               (second (second item)) ))
                           :id 
                           (if (> (length (second item)) 2)
                               (avoid-integer (second (second item))
                                              warning-stream)
                               nil )))
         )

        (t      ;;; must assume this is a matrix id now
          (setf 
            (minion-print inst)
            (minion-datum inst 'matrix (second item)))
          )
        ))

    (t      ;;; I am going to assume this is a constraint 
            ;;; which may generate an error later on of course
      
     (minion-add-constraint inst (first item)
                            (minion-recursive-process-items inst (cdr item)))
      
     )

    ))

(defun minion-stream-to-instance (s &optional (inst (make-minion)))

  (do ((start (read s t) (read s t))
       (version nil)
       )
      ((or (equal start 'minion) (equal start 'minionhelper))
       (setf version (read s t))
       (if (not (member version +minion-input-version-list+))
         (error 
           (format nil "Can only process up to input version ~A, Not ~A"
                   +minion-input-version+ version)))
       ))

  (do ((item (read s nil 'minion-end) (read s nil 'minion-end))
       )
      ((equal item 'minion-end)
       (minion-number-vars inst)
       inst                         ;;; return here 
       )

                                   ;;; process item here
      (minion-toplevel-item-process inst item s))
      )

(defun minion-file (infile &optional (outfile nil))
  (let ((inst nil))
    (with-open-file (instream infile :direction :input)
      (setf inst (minion-stream-to-instance instream))
      )
    (cond ((and (not outfile) (not (minion-filename inst))) 
           (minion-output inst))
          
          (outfile 
            (minion-printfile inst outfile))

          ((minion-filename inst)
           (minion-printfile inst (minion-filename inst)))

          (t 
            (error "how on earth did I get here???"))
          )))



(defun sbcl-make-executable (&optional (file "minion-helper-sbcl"))
  (save-lisp-and-die file :purify t :executable t))

;;; clisp stuff

(defun clisp-toplevel ()

   (cond ((null *ARGS*)
          (format *standard-output* "Usage: minion-helper -- inputfile outputfile~%")
          (format *standard-output* "~%    Note: the '--' is significant and must be entered")
          (format *standard-output* "~%    Standard input used if inputfile missing")
          (format *standard-output* "~%    Standard output used if outputfile missing~%")
          )

         (t
           (minion-file (first *ARGS*)
                        (if (endp (cdr *ARGS*)) nil (second *ARGS*)))
           )

         )

   (exit)
)

(defun clisp-make-executable (&optional (file "minion-helper")
                                        (init #'clisp-toplevel))
  "http://clisp.cons.org/impnotes/image.html"
  (saveinitmem file 
               :executable t 
               :quiet t
               :init-function init)
  )



