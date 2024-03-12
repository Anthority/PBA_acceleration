#include <vector>
#include <iostream>
#include <algorithm>
using namespace std;

pair<int, int> maxSubArraySum(vector<int> &nums)
{
    int n = nums.size();
    int maxSum = nums[0], curSum = nums[0];
    int start = 0, end = 0, tempStart = 0;

    for (int i = 1; i < n; i++)
    {
        if (curSum < 0)
        {
            curSum = nums[i];
            tempStart = i;
        }
        else
        {
            curSum += nums[i];
        }

        if (curSum > maxSum)
        {
            maxSum = curSum;
            start = tempStart;
            end = i;
        }
    }

    return {start, end};
}

int main()
{
    vector<int> nums = {-2, 1, -3, 4, -1, -2, 2, 1, -5, 4};
    pair<int, int> result = maxSubArraySum(nums);
    for (int i = result.first; i <= result.second; i++)
    {
        cout << nums[i] << " ";
    }
    return 0;
}