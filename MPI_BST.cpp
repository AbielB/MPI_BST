#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define ID 0
#define AGE 1 
#define LEFT 2
#define RIGHT 3
//ukuran tree = 40000
#define rows 40000

typedef struct
{
	int id;
	int age;
}BTree;

//locate mengalokasikan dan membagi data termasuk child kanan atau kiri menggunkan index
void locate(int index, int* node_array, BTree* node)
{
	node_array[index * 4 + ID] = node->id;
	node_array[index * 4 + AGE] = node->age;
	node_array[index * 4 + LEFT] = -1;
	node_array[index * 4 + RIGHT] = -1;
}

//insertNode akan memasukkan data ke dalam binary tree
void InsertNode(int root, BTree* node, int index, int* node_array)
{
	//jika node yang dimasukkan lebih besar node daripada root, akan masuk ke right
	if (node->age > node_array[root * 4 + AGE])
	{
		//jika node kanan kosong, alokasikan ke node kanan
		if (node_array[root * 4 + RIGHT] == -1)
		{
			node_array[root * 4 + RIGHT] = index;
			locate(index, node_array, node);
		}
		else // jika tidak kosong, cek lagi untuk child node kanan sebagai root
		{
			InsertNode(node_array[root * 4 + RIGHT], node, index, node_array);
		}
	}
	else //jika node yang dimasukkan lebih kecil daripada root, akan masuk ke left
	{
		if (node_array[root * 4 + LEFT] == -1) //jika node kiri kosong, alokasikan ke node kiri 
		{
			node_array[root * 4 + LEFT] = index;
			locate(index, node_array, node);
		}
		else // jika tidak kosong, cek lagi untuk child node kiri sebagai root
		{
			InsertNode(node_array[root * 4 + LEFT], node, index, node_array);
		}
	}
}

//prosedur untuk membuat tree
void  CreateTree(int size, int* node_array)
{
	int i;
	BTree* tree = (BTree*)malloc(sizeof(BTree));
	//ID node adalah angka random dari 0 sampai jumlah row
	tree->id = rand() % rows;
	//node age adalah angka random dari 1 - 40
	tree->age = rand() % 40;
	// alokasikan node pertama ke tree
	locate(0, node_array, tree);
	for (i = 1; i < size; i++)
	{
		BTree* node = (BTree*)malloc(sizeof(BTree));
		node->id = rand() % size;
		node->age = rand() % 60 + 10;
		//masukkan node-node lain ke dalam tree sebanyak size
		InsertNode(0, node, i, node_array);
	}

}


//prosedur untuk mencari data di dalam tree
void search(int age, int* node_array, int index)
{
	//jika node age = age yang dicari, print ID dan age
	if (node_array[index * 4 + AGE] == age)
	{
		printf("ID = %d, Age= %d\n", node_array[index * 4 + ID], node_array[index * 4 + AGE]);
	}
	// jika node kiri ada, lakukan pencarian di node kiri
	if (node_array[index * 4 + LEFT] != -1)
	{
		search(age, node_array, node_array[index * 4 + LEFT]);
	}
	// jika node kanan ada, lakukan pencarian di node kanan
	if (node_array[index * 4 + RIGHT] != -1)
	{
		search(age, node_array, node_array[index * 4 + RIGHT]);
	}

}

//prosedur check digunakan untuk mengecek node yang dicari
void check(int age, int* node_array, int index)
{
	//jika node age = age yang dicari, print ID dan age
	if (node_array[index * 4 + AGE] == age)
	{
		printf("ID = %d, Age= %d\n", node_array[index * 4 + ID], node_array[index * 4 + AGE]);
	}
}

//prosedur untuk membagi pekerjaan 
void get_more_work(int* work_list, int* num, int numP, int age, int* node_array)
{
	//jika num = jumlah processor, berhenti
	if ((*num) == numP)
	{
		return;
	}
	else
	{
		(*num)--;
		int root = work_list[*num];
		check(age, node_array, root);
		int left = node_array[root * 4 + LEFT];
		int right = node_array[root * 4 + LEFT];
		//jika node kiri ada, tambahkan work untuk node kiri
		if (left != -1)
		{
			work_list[*num] = left;
			(*num)++;
		}
		//jika node kanan ada, tambahkan work untuk node kanan
		if (right != -1)
		{
			work_list[*num] = right;
			(*num)++;
		}
		//recursive
		get_more_work(work_list, num, numP, age, node_array);
	}
}


void main(int argc, char** argv)
{

	int taskid, numtasks;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

	//membuat tree
	int tree[rows][4]; // 0 :ID, 1 :age, 2 :left , 3 : right

	if (!taskid)
	{
		CreateTree(rows, (int*)tree);
	}

	//lakukan pencarian node dengan age = 30
	int tragetAGE = 30;

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Bcast(tree, rows * 4, MPI_INT, 0, MPI_COMM_WORLD);

	double time;
	time = -MPI_Wtime();
	int* work_list = (int*)malloc(sizeof(int) * numtasks);

	if (!taskid)
	{
		int work = 1;
		work_list[0] = 0;
		get_more_work(work_list, &work, numtasks, tragetAGE, (int*)tree);
	}

	int begin_node;
	MPI_Barrier(MPI_COMM_WORLD);
	//melakukan scatter proses, setiap proses akan mendapatkan work
	MPI_Scatter(work_list, 1, MPI_INT, &begin_node, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
	printf("Menjalankan Search Pada Node : %d \n", begin_node);

	//setiap proses melakukan search
	search(tragetAGE, (int*)tree, begin_node);
	printf("Search Finished");
	MPI_Barrier(MPI_COMM_WORLD);
	time += MPI_Wtime();

	//print waktu jika sudah selesai
	if (!taskid)
		printf("Time:%lf, numtasks:%d\n", time, numtasks);
	free(work_list);
	MPI_Finalize();

}
