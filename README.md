# Parallel MPI Project

<div align="center">

**🌐 Language / اللغة**

[🇸🇦 العربية](#ar) &nbsp;|&nbsp; [🇬🇧 English](#en)

</div>

---

<a name="ar"></a>
<div dir="rtl">

## 📘 التوثيق القصير — العربية

### جدول المحتويات
- [نظرة عامة على التصميم](#ar-design)
- [استراتيجيات التواصل](#ar-comm)
- [شرح الـ Deadlock](#ar-deadlock)
- [ملاحظات الأداء](#ar-perf)
- [لقطات تجريبية](#ar-screenshots)
- [متطلبات التشغيل والتثبيت](#ar-setup)

---

<a name="ar-design"></a>
### أ. نظرة عامة على التصميم

المشروع عبارة عن تطبيق C++ يستخدم **MPI (Message Passing Interface)** لتنفيذ خمس خوارزميات متوازية. يتم توزيع البيانات على عمليات متعددة، وتتبادل هذه العمليات الرسائل لإنجاز الحوسبة بشكل موازٍ، ثم تُجمَّع النتائج في العملية الرئيسية (rank 0).

**الخوارزميات المنفَّذة:**

| # | الاسم | الفئة | التقنية |
|---|-------|--------|---------|
| 1 | Heat Diffusion 1D | Category A | Blocking (`MPI_Send/Recv`) |
| 2 | Game of Life | Category A | Non-Blocking (`MPI_Isend/Irecv`) |
| 3 | Odd-Even Sort | Category B | `MPI_Sendrecv` |
| 4 | Prefix Sum | Category B | `MPI_Comm_split` + `MPI_Scan` |
| 5 | Heat Diffusion 2D *(Bonus)* | Bonus | Cartesian Topology |

**تدفق التنفيذ:**
1. العملية 0 تقرأ ملف الإدخال وتوزّع البيانات على جميع العمليات (`MPI_Scatterv`).
2. كل عملية تُجري الحوسبة على جزئها، مع تبادل بيانات الحدود (Halo Exchange) مع الجيران.
3. بعد الانتهاء، تُجمَّع النتائج في العملية 0 (`MPI_Gatherv`) وتُحفظ في ملف.

---

<a name="ar-comm"></a>
### ب. استراتيجيات التواصل

**1. Blocking Communication — Heat Diffusion 1D**
- تستخدم `MPI_Send` و`MPI_Recv` لتبادل صفوف الحدود (halo rows) بين العمليات المتجاورة.
- **استراتيجية تجنب الـ Deadlock:** يتبع الكود نظام **Odd-Even Ordering**؛ العمليات ذات الأرقام الزوجية ترسل أولاً ثم تستقبل، والعمليات الفردية تستقبل أولاً ثم ترسل.

**2. Non-Blocking Communication — Game of Life**
- تستخدم `MPI_Isend` و`MPI_Irecv` لبدء تبادل halo rows بشكل غير متزامن.
- يستدعي الكود `MPI_Waitall` للانتظار حتى تكتمل جميع الرسائل قبل بدء الحوسبة.
- الميزة: تداخل الاتصال مع الحوسبة يُحسّن الأداء نظرياً.

**3. MPI_Sendrecv — Odd-Even Sort**
- يستخدم `MPI_Sendrecv` الذي يجمع الإرسال والاستقبال في عملية ذرية واحدة.
- يتجنب تلقائياً مشكلة الـ Deadlock دون الحاجة لترتيب يدوي.

**4. Communicator Split — Prefix Sum**
- يستخدم `MPI_Comm_split` لتقسيم العمليات إلى مجموعتين (فردية وزوجية).
- يُطبَّق `MPI_Scan` داخل كل مجموعة لحساب Prefix Sum جزئي.

**5. Cartesian Topology — Heat Diffusion 2D**
- يستخدم `MPI_Cart_create` لترتيب العمليات في شبكة ثنائية الأبعاد.
- `MPI_Cart_shift` يُحدد الجيران في الاتجاهين (أفقي وعمودي).
- يتم تبادل الحدود في الاتجاهات الأربعة (شمال، جنوب، شرق، غرب).

---

<a name="ar-deadlock"></a>
### ج. شرح الـ Deadlock

**ما هو الـ Deadlock في MPI؟**
يحدث الـ Deadlock عندما تنتظر جميع العمليات بعضها البعض دون أن تُكمل أي منها، مما يُجمّد التنفيذ إلى الأبد.

**مثال على السيناريو الخطر (في Heat Diffusion 1D):**
```cpp
// ❌ هذا الكود يمكن أن يتسبب في Deadlock
// كل العمليات تُرسل أولاً → لا أحد يستقبل → الجميع ينتظر
MPI_Send(..., neighbor, ...);   // يبلوك حتى يستقبل أحد
MPI_Recv(..., neighbor, ...);   // لن نصل هنا أبداً
```

**الحل المُطبَّق: Odd-Even Phase Ordering**
```cpp
if (rank % 2 == 0) {
    MPI_Send(...);  // الزوجيون يرسلون أولاً
    MPI_Recv(...);
} else {
    MPI_Recv(...);  // الفرديون يستقبلون أولاً
    MPI_Send(...);
}
```
بهذا الترتيب، لا توجد عملية تنتظر عملية أخرى تنتظرها هي نفسها.

**الحل في Odd-Even Sort: `MPI_Sendrecv`**
هذه الدالة تُرسل وتستقبل في نفس الوقت بشكل ذري، مما يلغي احتمال الـ Deadlock كلياً.

---

<a name="ar-perf"></a>
### د. ملاحظات الأداء

**قياس الزمن:**
يستخدم الكود `MPI_Wtime()` لقياس وقت التنفيذ الكلي من بداية التوزيع حتى نهاية التجميع.

**الملاحظات العامة:**

| الخوارزمية | 2 Cores | 4 Cores | الملاحظة |
|------------|---------|---------|----------|
| Heat Diffusion 1D | أسرع من core واحد | أسرع من 2 cores | تحسن خطي مع زيادة الـ cores |
| Game of Life | ✓ | ✓ | Non-blocking يُقلل وقت الانتظار |
| Odd-Even Sort | ✓ | ✓ | الأداء يعتمد على توزيع البيانات |
| Prefix Sum | ✓ | ✓ | `MPI_Scan` فعّال للبيانات الكبيرة |
| Heat Diffusion 2D | ✓ | ✓ | الأسرع عند عدد cores يُشكّل مربعاً كاملاً |

**عوامل تؤثر على الأداء:**
- حجم البيانات: كلما زاد حجم الـ grid أو الـ array، زادت الفائدة من التوازي.
- عدد العمليات: زيادة العمليات تُقلل وقت الحوسبة لكن تزيد تكلفة التواصل.
- توازن الحمل: `MPI_Scatterv` يُوزّع البيانات بشكل متوازن حتى لو لم يكن الحجم قابلاً للقسمة بالتساوي.

---

<a name="ar-screenshots"></a>
### هـ. لقطات تجريبية

#### Category A — Heat Diffusion & Game of Life

| 2 Cores | 4 Cores |
|---------|---------|
| ![Category A - 2 Cores](Demo%20Screenshots/Category%20A%202%20cores.png) | ![Category A - 4 Cores](Demo%20Screenshots/category%20A%204%20cores.png) |

#### Category B — Odd-Even Sort & Prefix Sum

| 2 Cores | 4 Cores |
|---------|---------|
| ![Category B - 2 Cores](Demo%20Screenshots/category%20B%202%20cores.png) | ![Category B - 4 Cores](Demo%20Screenshots/category%20B%204%20cores.png) |

#### Bonus — 2D Cartesian Decomposition

| 2 Cores | 4 Cores |
|---------|---------|
| ![2D Cart - 2 Cores](Demo%20Screenshots/2D%20cart%20decomposition%202%20cores.png) | ![2D Cart - 4 Cores](Demo%20Screenshots/2D%20cart%20decomposition%204%20cores.png) |

---

<a name="ar-setup"></a>
### متطلبات التشغيل والتثبيت

**المتطلبات:**
- Visual Studio (مع دعم C++)
- MS-MPI أو OpenMPI

**خطوات التشغيل:**
1. شغّل `Source1.cpp` لتوليد ملفات الإدخال (`heat_grid.txt`, `gol_grid.txt`, `large_array.txt`).
2. افتح `paralel project.sln` في Visual Studio وابنِ المشروع.
3. شغّل البرنامج عبر MPI:
   ```bash
   mpiexec -n 4 "paralel project.exe"
   ```
4. اختر رقم الخوارزمية من القائمة التفاعلية.

</div>

---

<a name="en"></a>

## 📘 Short Documentation — English

### Table of Contents
- [Design Overview](#en-design)
- [Communication Strategies](#en-comm)
- [Deadlock Explanation](#en-deadlock)
- [Performance Observations](#en-perf)
- [Demo Screenshots](#en-screenshots)
- [Setup & Requirements](#en-setup)

---

<a name="en-design"></a>
### a. Design Overview

This project is a C++ application that uses **MPI (Message Passing Interface)** to implement five parallel algorithms. Data is distributed across multiple processes, which exchange messages to perform computation in parallel. Results are gathered back to the root process (rank 0) and written to output files.

**Implemented Algorithms:**

| # | Name | Category | Technique |
|---|------|----------|-----------|
| 1 | Heat Diffusion 1D | Category A | Blocking (`MPI_Send/Recv`) |
| 2 | Game of Life | Category A | Non-Blocking (`MPI_Isend/Irecv`) |
| 3 | Odd-Even Sort | Category B | `MPI_Sendrecv` |
| 4 | Prefix Sum | Category B | `MPI_Comm_split` + `MPI_Scan` |
| 5 | Heat Diffusion 2D *(Bonus)* | Bonus | Cartesian Topology |

**Execution Flow:**
1. Rank 0 reads the input file and distributes data to all processes using `MPI_Scatterv`.
2. Each process computes on its local chunk, exchanging boundary rows (Halo Exchange) with neighbors.
3. After computation, results are gathered by rank 0 via `MPI_Gatherv` and saved to a file.

---

<a name="en-comm"></a>
### b. Communication Strategies

**1. Blocking Communication — Heat Diffusion 1D**
- Uses `MPI_Send` and `MPI_Recv` to exchange halo rows between neighboring processes.
- **Deadlock prevention strategy:** Odd-Even Ordering — even-ranked processes send first then receive; odd-ranked processes receive first then send.

**2. Non-Blocking Communication — Game of Life**
- Uses `MPI_Isend` and `MPI_Irecv` to initiate halo exchange asynchronously.
- `MPI_Waitall` is called to ensure all messages complete before computation begins.
- Benefit: communication and computation can theoretically overlap.

**3. MPI_Sendrecv — Odd-Even Sort**
- `MPI_Sendrecv` atomically combines send and receive in a single call.
- Inherently deadlock-free without requiring manual ordering.

**4. Communicator Split — Prefix Sum**
- `MPI_Comm_split` divides processes into two sub-communicators (even/odd ranks).
- `MPI_Scan` computes a partial prefix sum within each sub-group.

**5. Cartesian Topology — Heat Diffusion 2D**
- `MPI_Cart_create` arranges processes in a 2D grid.
- `MPI_Cart_shift` identifies neighbors in both dimensions.
- Halo exchange occurs in all four directions (north, south, east, west).

---

<a name="en-deadlock"></a>
### c. Deadlock Explanation

**What is an MPI Deadlock?**
A deadlock occurs when all processes are waiting for each other simultaneously, causing execution to freeze indefinitely.

**Example of a dangerous scenario (Heat Diffusion 1D):**
```cpp
// ❌ This code can cause a deadlock
// All processes send first → no one receives → everyone waits forever
MPI_Send(..., neighbor, ...);  // blocks until someone receives
MPI_Recv(..., neighbor, ...);  // never reached
```

**Solution applied: Odd-Even Phase Ordering**
```cpp
if (rank % 2 == 0) {
    MPI_Send(...);  // even ranks send first
    MPI_Recv(...);
} else {
    MPI_Recv(...);  // odd ranks receive first
    MPI_Send(...);
}
```
This ordering ensures no process is waiting for another that is also waiting for it.

**Solution in Odd-Even Sort: `MPI_Sendrecv`**
This function performs send and receive atomically and simultaneously, completely eliminating the possibility of deadlock.

---

<a name="en-perf"></a>
### d. Performance Observations

**Timing:**
The code uses `MPI_Wtime()` to measure total wall-clock time from data scatter to result gather.

**General Observations:**

| Algorithm | 2 Cores | 4 Cores | Note |
|-----------|---------|---------|------|
| Heat Diffusion 1D | Faster than 1 core | Faster than 2 cores | Near-linear scaling |
| Game of Life | ✓ | ✓ | Non-blocking reduces idle time |
| Odd-Even Sort | ✓ | ✓ | Performance depends on data distribution |
| Prefix Sum | ✓ | ✓ | `MPI_Scan` is efficient for large arrays |
| Heat Diffusion 2D | ✓ | ✓ | Best when core count forms a perfect square |

**Factors affecting performance:**
- **Data size:** Larger grids/arrays benefit more from parallelism.
- **Process count:** More processes reduce computation time but increase communication overhead.
- **Load balance:** `MPI_Scatterv` distributes data evenly even when the size is not divisible by the number of processes.

---

<a name="en-screenshots"></a>
### e. Demo Screenshots

#### Category A — Heat Diffusion & Game of Life

| 2 Cores | 4 Cores |
|---------|---------|
| ![Category A - 2 Cores](Demo%20Screenshots/Category%20A%202%20cores.png) | ![Category A - 4 Cores](Demo%20Screenshots/category%20A%204%20cores.png) |

#### Category B — Odd-Even Sort & Prefix Sum

| 2 Cores | 4 Cores |
|---------|---------|
| ![Category B - 2 Cores](Demo%20Screenshots/category%20B%202%20cores.png) | ![Category B - 4 Cores](Demo%20Screenshots/category%20B%204%20cores.png) |

#### Bonus — 2D Cartesian Decomposition

| 2 Cores | 4 Cores |
|---------|---------|
| ![2D Cart - 2 Cores](Demo%20Screenshots/2D%20cart%20decomposition%202%20cores.png) | ![2D Cart - 4 Cores](Demo%20Screenshots/2D%20cart%20decomposition%204%20cores.png) |

---

<a name="en-setup"></a>
### Setup & Requirements

**Requirements:**
- Visual Studio (with C++ workload)
- MS-MPI or OpenMPI runtime

**Steps:**
1. Run `Source1.cpp` to generate input files (`heat_grid.txt`, `gol_grid.txt`, `large_array.txt`).
2. Open `paralel project.sln` in Visual Studio and build the project.
3. Run the program via MPI:
   ```bash
   mpiexec -n 4 "paralel project.exe"
   ```
4. Choose an algorithm number from the interactive menu.
