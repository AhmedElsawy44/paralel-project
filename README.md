# Parallel Project

مشروع C++ يعتمد على **MPI** لتنفيذ مجموعة من خوارزميات المعالجة المتوازية على البيانات الشبكية والبيانات الضخمة.

## المحتوى
- [فكرة المشروع](#فكرة-المشروع)
- [الخوارزميات المتاحة](#الخوارزميات-المتاحة)
- [متطلبات التشغيل](#متطلبات-التشغيل)
- [التشغيل](#التشغيل)
- [ملفات الإدخال والإخراج](#ملفات-الإدخال-والإخراج)
- [هيكل المشروع](#هيكل-المشروع)
- [ملاحظات مهمة](#ملاحظات-مهمة)

## فكرة المشروع
التطبيق يعرض قائمة تشغيل تفاعلية، ثم ينفّذ واحدة من خوارزميات التوازي باستخدام مجموعة من العمليات (MPI ranks)، مع قياس زمن التنفيذ في النهاية.

## الخوارزميات المتاحة
1. **Heat Diffusion (1D Blocking)**  
   توزيع المصفوفة على العمليات مع تبادل حدودي (Halo Exchange) باستخدام `MPI_Send / MPI_Recv` بطريقة تتجنب الـ deadlock.

2. **Game of Life (1D Non-Blocking)**  
   نفس فكرة التوزيع مع تبادل حدودي باستخدام `MPI_Isend / MPI_Irecv` ثم `MPI_Waitall`.

3. **Odd-Even Sort (Big Data)**  
   فرز متوازي للبيانات باستخدام أسلوب Odd-Even Transposition مع دمج جزئي بين العمليات.

4. **Prefix Sum (Big Data)**  
   حساب Prefix Sum باستخدام تقسيم البيانات وعمليات تجميع MPI، مع تقسيم communicator فرعي.

5. **Heat Diffusion (2D Decomposition - Bonus)**  
   توزيع ثنائي الأبعاد باستخدام Cartesian topology (`MPI_Cart_create`) وتبادل حدودي في الاتجاهات الأربعة.

## متطلبات التشغيل
- C++ Compiler
- MPI Runtime + MPI Compiler (مثل OpenMPI أو MS-MPI)
- بيئة تدعم بناء مشاريع C++ (Visual Studio على Windows لهذا الحل)

> **مهم:** ملف المشروع الحالي (`.sln` / `.vcxproj`) مهيأ لبيئة Visual Studio.

## التشغيل
### 1) تجهيز بيانات الإدخال
يمكن توليد الملفات التالية باستخدام ملف:
- `Source1.cpp` (أو النسخة الموجودة داخل مجلد `paralel project/Source.cpp`)

الملفات التي يتم توليدها:
- `heat_grid.txt`
- `gol_grid.txt`
- `large_array.txt`

### 2) بناء المشروع
افتح `paralel project.sln` في Visual Studio ثم Build للمشروع.

### 3) تشغيل البرنامج على أكثر من عملية
بعد البناء، شغّل البرنامج عبر MPI (مثال عام):

```bash
mpiexec -n 4 paralel_project.exe
```

ثم اختر رقم الخوارزمية من القائمة داخل البرنامج.

## ملفات الإدخال والإخراج
### Input
- `heat_grid.txt`: شبكة حرارة (rows, cols ثم القيم)
- `gol_grid.txt`: شبكة Game of Life (0 و 1)
- `large_array.txt`: مصفوفة كبيرة (الحجم ثم العناصر)

### Output
- `heat_result.txt`
- `game_of_life_result.txt`
- `sorted_array_result.txt`
- `prefix_sum_result.txt`
- `heat_result_2d.txt`

## هيكل المشروع
- `paralel project.sln` : ملف الحل (Visual Studio Solution)
- `paralel project/paralel project.vcxproj` : إعدادات المشروع
- `paralel project/paralel project.cpp` : الكود الرئيسي والخوارزميات
- `Source1.cpp` : مولد بيانات الإدخال
- `paralel project/Source.cpp` : نسخة إضافية من مولد البيانات

## ملاحظات مهمة
- تأكد أن ملفات الإدخال موجودة في نفس مسار التشغيل التنفيذي.
- اختيار عدد عمليات MPI يؤثر بشكل مباشر على الأداء.
- بعض مخرجات الملفات تحفظ أول جزء فقط من النتائج (مثل أول 100 عنصر في بعض الحالات) لتقليل حجم الإخراج.
