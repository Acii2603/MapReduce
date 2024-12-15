
# 🛠️ Inverted Index Using MapReduce with Pthreads

---

## 🚀 **Overview**
This implementation processes multiple text files to compute an **inverted index** using the **MapReduce paradigm**. The solution is designed for **parallel processing** using **Pthreads**. Here's how it works:

1. **Input Files**: A list of text files is provided as input.
2. **Mapping Phase**:
   - Mapper threads process the files to extract words.
   - Words are normalized (converted to lowercase and stripped of non-alphabetic characters).
   - Each word is stored in a **temporary inverted index**.
3. **Reducing Phase**:
   - Reducer threads merge temporary indexes into a **global inverted index**.
   - Words are grouped alphabetically, and their occurrences in files are counted.
4. **Output**:
   - Results are written to separate output files (`a.txt`, `b.txt`, ...).

---

## 🧩 **Design Highlights**

### 🔄 **Dynamic Task Allocation**
Mappers fetch unprocessed files dynamically, ensuring better workload distribution and minimizing idle time.

### 🔒 **Thread-Safe Operations**
Shared resources like the global inverted index and file queue are protected using **mutexes** and **condition variables**, ensuring data integrity in a multithreaded environment.

### 🗂️ **Scalable Data Structures**
1. **FileIDSet**:
   - A dynamic set to store unique file IDs.
   - Ensures efficient addition and lookup.

2. **InvertedIndex**:
   - A hash table-based index mapping words to FileIDSets.
   - Allows fast word insertion and retrieval.

### ⚡ **Hashing with FNV-1a**
Efficient hash function (`FNV-1a`) is used to distribute words across hash buckets in the inverted index.

---

## 🛠️ **How It Works**

### **1. Initialization**
- Input arguments (`numar_mapperi`, `numar_reduceri`, `fisier_intrare`) are parsed.
- The file list is loaded, and resources like the global inverted index and synchronization primitives are initialized.

---

### **2. Mapping Phase (Map)**
Each Mapper thread:
1. Dynamically fetches unprocessed files.
2. Reads the file line by line, extracting words.
3. Normalizes the words and inserts them into a **temporary inverted index**.

---

### **3. Reducing Phase (Reduce)**
Each Reducer thread:
1. Waits until all Mappers complete their work.
2. Processes the global inverted index for its assigned letter range (e.g., `a-m`).
3. Writes results to an output file (`a.txt`, `b.txt`, ...).

---

## 📂 **Key Data Structures**

### 🗃️ **FileIDSet**
- A dynamic set that stores unique file IDs.
- Automatically resizes as needed.

### 📑 **InvertedIndex**
- A hash table where each bucket points to a linked list of entries.
- Each entry maps a word to a `FileIDSet`.

---

## 🔧 **Implementation Components**

### **`normalize_word`**
- Converts words to lowercase and removes non-alphabetic characters.

### **`process_file`**
- Reads a file, extracts words, and populates a temporary inverted index.

### **`merge_temp_index_into_global`**
- Merges Mapper-generated temporary indexes into the global index, ensuring thread safety.

### **`write_results_to_files`**
- Outputs the Reducer's results for each letter to a separate file.

---

## 📋 **Input and Output**

### **Input**
A text file listing the paths of files to be processed. Each file contains plain text.

**Example Input File:**
```txt
3
file1.txt
file2.txt
file3.txt
```

**Example File Content (file1.txt):**
```txt
The bright sun shines in the blue sky.
```

### **Output**
- Each letter has its own file (`a.txt`, `b.txt`, ...).
- Each file contains words starting with that letter and the IDs of files where the words appear.

**Example Output (b.txt):**
```txt
blue:[1 2]
bright:[1]
brightly:[3]
```

---

## 🧠 **Challenges and Solutions**

### **1. Dynamic Work Distribution**
- **Challenge**: Ensuring Mapper threads are efficiently utilized.
- **Solution**: Use a shared file queue with dynamic allocation to Mappers.

### **2. Thread Synchronization**
- **Challenge**: Safely merging temporary indexes into the global index.
- **Solution**: Mutex-protected operations for shared resources.

### **3. Large Data Handling**
- **Challenge**: Managing memory for large input files and indexes.
- **Solution**: Dynamic resizing of data structures (`FileIDSet` and `InvertedIndex`).

---

## 🚀 **Performance Optimization**

1. **Load Balancing**:
   - Mappers dynamically fetch unprocessed files, preventing idle threads.

2. **Efficient Hashing**:
   - The FNV-1a hash ensures uniform distribution across hash buckets.

3. **Parallelism**:
   - Independent Mapper and Reducer threads maximize CPU utilization.

---

## 🛡️ **Testing and Validation**

### **1. Automated Testing**
A `checker.sh` script validates the implementation by comparing program output with expected results.

### **2. Stress Testing**
- Input large datasets with many files and words to test scalability.
- Run with varying numbers of Mappers and Reducers to measure performance.

---

## 👨‍💻 © **Copyright**
**Ionita Alexandru Andrei**  
**Grupa 332CA**  
All rights reserved.
